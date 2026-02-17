#include "Entity.hpp"

#include "../Utils/Log.hpp"
#include "GameData.hpp"
#include "GameInterfaces.hpp"
#include "Schema.hpp"

#include <entity2/concreteentitylist.h>
#include <entity2/entityidentity.h>
#include <entity2/entityinstance.h>
#include <entity2/entitysystem.h>

using namespace AdminSystem::Utils;

namespace AdminSystem::Sdk
{

void EntitySystem::ResolveSchemaOffsets()
{
    if (_schemaOffsetsResolved)
        return;

    auto& schema = SchemaService::Instance();
    _offsetPlayerPawn = schema.GetOffset("CCSPlayerController", "m_hPlayerPawn");
    _offsetMovementServices = schema.GetOffset("CBasePlayerPawn", "m_pMovementServices");
    _offsetButtons = schema.GetOffset("CPlayer_MovementServices", "m_nButtons");
    _offsetButtonStates = schema.GetOffset("CInButtonState", "m_pButtonStates");

    _schemaOffsetsResolved = true;

    if (_offsetPlayerPawn >= 0 && _offsetMovementServices >= 0 && _offsetButtons >= 0 && _offsetButtonStates >= 0)
    {
        Log::Info("Button access chain resolved via schema:");
        Log::Info("  Controller + 0x{:X} -> Pawn + 0x{:X} -> MovementServices + 0x{:X} -> Buttons + 0x{:X}",
                  _offsetPlayerPawn, _offsetMovementServices, _offsetButtons, _offsetButtonStates);
    }
    else
    {
        Log::Warn("Some schema offsets not resolved. Button detection may not work.");
    }
}

bool EntitySystem::Initialize()
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

CGameEntitySystem* EntitySystem::GetEntitySystem()
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

CEntityIdentity* EntitySystem::GetEntityIdentityByIndex(CGameEntitySystem* pSys, int index)
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

CEntityInstance* EntitySystem::ResolveEntityHandle(uint32_t handle)
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

CEntityInstance* EntitySystem::GetPlayerController(int slot)
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

uint64_t EntitySystem::GetPlayerButtons(int slot)
{
    // Resolve schema offsets on first call (deferred because schema may not be ready at init)
    if (!_schemaOffsetsResolved)
        ResolveSchemaOffsets();

    if (_offsetPlayerPawn < 0 || _offsetMovementServices < 0 || _offsetButtons < 0 || _offsetButtonStates < 0)
        return 0;

    // Step 1: Get player controller entity
    CEntityInstance* pController = GetPlayerController(slot);
    if (!pController)
        return 0;

    auto* pCtrlBase = reinterpret_cast<uint8_t*>(pController);

    // Step 2: Read m_hPlayerPawn handle (CHandle<CCSPlayerPawn> = uint32)
    uint32_t hPawn = *reinterpret_cast<uint32_t*>(pCtrlBase + _offsetPlayerPawn);
    CEntityInstance* pPawn = ResolveEntityHandle(hPawn);
    if (!pPawn)
        return 0;

    auto* pPawnBase = reinterpret_cast<uint8_t*>(pPawn);

    // Step 3: Read m_pMovementServices pointer
    auto* pMovementServices = *reinterpret_cast<uint8_t**>(pPawnBase + _offsetMovementServices);
    if (!pMovementServices)
        return 0;

    // Step 4: Read m_pButtonStates from CInButtonState (embedded at m_nButtons offset)
    // CInButtonState::m_pButtonStates is an array of 3 uint64:
    //   [0] = currently held buttons
    //   [1] = buttons that changed this frame
    //   [2] = scroll/other
    auto* pButtonStates = reinterpret_cast<uint64_t*>(pMovementServices + _offsetButtons + _offsetButtonStates);

    return pButtonStates[0];  // Current held buttons
}

bool EntitySystem::IsPlayerSlotValid(int slot)
{
    return GetPlayerController(slot) != nullptr;
}

}  // namespace AdminSystem::Sdk
