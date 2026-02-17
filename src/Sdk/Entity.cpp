#include "Entity.hpp"

#include "../Utils/Log.hpp"
#include "GameData.hpp"
#include "GameInterfaces.hpp"
#include "Schema.hpp"

#include <ISmmPlugin.h>

#include <entity2/concreteentitylist.h>
#include <entity2/entityidentity.h>
#include <entity2/entityinstance.h>
#include <entity2/entitysystem.h>

using namespace AdminSystem::Utils;

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace AdminSystem::Sdk
{

// Schema-resolved offsets for the button access chain (cached after first lookup)
static int sOffsetPlayerPawn = -1;        // CCSPlayerController::m_hPlayerPawn
static int sOffsetMovementServices = -1;  // CBasePlayerPawn::m_pMovementServices
static int sOffsetButtons = -1;           // CPlayer_MovementServices::m_nButtons
static int sOffsetButtonStates = -1;      // CInButtonState::m_pButtonStates
static bool sSchemaOffsetsResolved = false;

static void ResolveSchemaOffsets()
{
    if (sSchemaOffsetsResolved)
        return;

    sOffsetPlayerPawn = GetSchemaOffset("CCSPlayerController", "m_hPlayerPawn");
    sOffsetMovementServices = GetSchemaOffset("CBasePlayerPawn", "m_pMovementServices");
    sOffsetButtons = GetSchemaOffset("CPlayer_MovementServices", "m_nButtons");
    sOffsetButtonStates = GetSchemaOffset("CInButtonState", "m_pButtonStates");

    sSchemaOffsetsResolved = true;

    if (sOffsetPlayerPawn >= 0 && sOffsetMovementServices >= 0 && sOffsetButtons >= 0 && sOffsetButtonStates >= 0)
    {
        Log::Info("Button access chain resolved via schema:");
        Log::Info("  Controller + 0x{:X} -> Pawn + 0x{:X} -> MovementServices + 0x{:X} -> Buttons + 0x{:X}",
                  sOffsetPlayerPawn, sOffsetMovementServices, sOffsetButtons, sOffsetButtonStates);
    }
    else
    {
        Log::Warn("Some schema offsets not resolved. Button detection may not work.");
    }
}

bool InitEntitySystem()
{
    auto& interfaces = GameInterfaces::Instance();

    if (!interfaces.GameResourceService)
    {
        Log::Warn("IGameResourceService not available.");
    }

    // Get the GameEntitySystem offset from GameData (already parsed)
    int offsetGameEntitySystem = GameData::Instance().GetOffset("GameEntitySystem");

    if (offsetGameEntitySystem < 0)
    {
        Log::Warn("GameEntitySystem offset not found in gamedata.");
    }
    else
    {
        Log::Info("Gamedata loaded (entity system offset: {}).", offsetGameEntitySystem);
    }

    // Resolve entity system pointer from IGameResourceService + offset
    if (interfaces.GameResourceService && offsetGameEntitySystem >= 0)
    {
        interfaces.EntitySystem = *reinterpret_cast<CGameEntitySystem**>(
            reinterpret_cast<uintptr_t>(interfaces.GameResourceService) + offsetGameEntitySystem);
    }

    if (interfaces.EntitySystem)
    {
        Log::Info("Entity system initialized.");
    }
    else
    {
        Log::Warn("Entity system not available. Menu button detection disabled.");
    }

    return true;
}

CGameEntitySystem* GetEntitySystem()
{
    auto& interfaces = GameInterfaces::Instance();

    // Lazy re-resolve if not yet available (entity system may not exist until map load)
    if (!interfaces.EntitySystem && interfaces.GameResourceService)
    {
        int offsetGameEntitySystem = GameData::Instance().GetOffset("GameEntitySystem");
        if (offsetGameEntitySystem >= 0)
        {
            interfaces.EntitySystem = *reinterpret_cast<CGameEntitySystem**>(
                reinterpret_cast<uintptr_t>(interfaces.GameResourceService) + offsetGameEntitySystem);
        }
    }
    return interfaces.EntitySystem;
}

// Our own implementation of entity identity lookup via the chunk list.
// Avoids calling CEntitySystem::GetEntityIdentity which is a non-virtual
// function in the game binary that we can't link against.
static CEntityIdentity* GetEntityIdentityByIndex(CGameEntitySystem* pSys, int index)
{
    if (!pSys || index < 0 || index >= MAX_TOTAL_ENTITIES)
        return nullptr;

    int chunk = index / MAX_ENTITIES_IN_LIST;
    int offset = index % MAX_ENTITIES_IN_LIST;

    CEntityIdentity* pChunk = pSys->m_EntityList.m_pIdentityChunks[chunk];
    if (!pChunk)
        return nullptr;

    return &pChunk[offset];
}

// Resolve a CHandle (entity handle) to a CEntityInstance pointer.
// CHandle stores entity index in the low 15 bits.
static CEntityInstance* ResolveEntityHandle(uint32_t handle)
{
    if (handle == 0xFFFFFFFF)
        return nullptr;

    int entryIndex = handle & 0x7FFF;

    auto* pSys = GetEntitySystem();
    if (!pSys)
        return nullptr;

    CEntityIdentity* pIdentity = GetEntityIdentityByIndex(pSys, entryIndex);
    if (!pIdentity)
        return nullptr;

    return pIdentity->m_pInstance;
}

CEntityInstance* GetPlayerController(int slot)
{
    auto* pSys = GetEntitySystem();
    if (!pSys || slot < 0 || slot >= MaxPlayers)
        return nullptr;

    // Player controllers are at entity indices (slot + 1)
    CEntityIdentity* pIdentity = GetEntityIdentityByIndex(pSys, slot + 1);
    if (!pIdentity)
        return nullptr;

    return pIdentity->m_pInstance;
}

uint64_t GetPlayerButtons(int slot)
{
    // Resolve schema offsets on first call (deferred because schema may not be ready at init)
    if (!sSchemaOffsetsResolved)
        ResolveSchemaOffsets();

    if (sOffsetPlayerPawn < 0 || sOffsetMovementServices < 0 || sOffsetButtons < 0 || sOffsetButtonStates < 0)
        return 0;

    // Step 1: Get player controller entity
    CEntityInstance* pController = GetPlayerController(slot);
    if (!pController)
        return 0;

    auto* pCtrlBase = reinterpret_cast<uint8_t*>(pController);

    // Step 2: Read m_hPlayerPawn handle (CHandle<CCSPlayerPawn> = uint32)
    uint32_t hPawn = *reinterpret_cast<uint32_t*>(pCtrlBase + sOffsetPlayerPawn);
    CEntityInstance* pPawn = ResolveEntityHandle(hPawn);
    if (!pPawn)
        return 0;

    auto* pPawnBase = reinterpret_cast<uint8_t*>(pPawn);

    // Step 3: Read m_pMovementServices pointer
    auto* pMovementServices = *reinterpret_cast<uint8_t**>(pPawnBase + sOffsetMovementServices);
    if (!pMovementServices)
        return 0;

    // Step 4: Read m_pButtonStates from CInButtonState (embedded at m_nButtons offset)
    // CInButtonState::m_pButtonStates is an array of 3 uint64:
    //   [0] = currently held buttons
    //   [1] = buttons that changed this frame
    //   [2] = scroll/other
    auto* pButtonStates = reinterpret_cast<uint64_t*>(pMovementServices + sOffsetButtons + sOffsetButtonStates);

    return pButtonStates[0];  // Current held buttons
}

bool IsPlayerSlotValid(int slot)
{
    return GetPlayerController(slot) != nullptr;
}

}  // namespace AdminSystem::Sdk
