#include "entity.h"
#include "schema.h"

#include <ISmmPlugin.h>
#include <entity2/entitysystem.h>
#include <entity2/entityinstance.h>
#include <entity2/entityidentity.h>
#include <entity2/concreteentitylist.h>
#include <interfaces/interfaces.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace sdk {

CGameEntitySystem* g_pEntitySystem = nullptr;


// GameEntitySystem offset (from gamedata — this is NOT in the schema system)
static int s_offsetGameEntitySystem = -1;

// Schema-resolved offsets for the button access chain (cached after first lookup)
static int s_offsetPlayerPawn = -1;       // CCSPlayerController::m_hPlayerPawn
static int s_offsetMovementServices = -1; // CBasePlayerPawn::m_pMovementServices
static int s_offsetButtons = -1;          // CPlayer_MovementServices::m_nButtons
static int s_offsetButtonStates = -1;     // CInButtonState::m_pButtonStates
static bool s_schemaOffsetsResolved = false;

static void ResolveSchemaOffsets()
{
    if (s_schemaOffsetsResolved)
        return;

    s_offsetPlayerPawn = GetSchemaOffset("CCSPlayerController", "m_hPlayerPawn");
    s_offsetMovementServices = GetSchemaOffset("CBasePlayerPawn", "m_pMovementServices");
    s_offsetButtons = GetSchemaOffset("CPlayer_MovementServices", "m_nButtons");
    s_offsetButtonStates = GetSchemaOffset("CInButtonState", "m_pButtonStates");

    s_schemaOffsetsResolved = true;

    if (s_offsetPlayerPawn >= 0 && s_offsetMovementServices >= 0 &&
        s_offsetButtons >= 0 && s_offsetButtonStates >= 0)
    {
        META_CONPRINTF("[AdminSystem] Button access chain resolved via schema:\n");
        META_CONPRINTF("  Controller + 0x%X -> Pawn + 0x%X -> MovementServices + 0x%X -> Buttons + 0x%X\n",
                       s_offsetPlayerPawn, s_offsetMovementServices, s_offsetButtons, s_offsetButtonStates);
    }
    else
    {
        META_CONPRINTF("[AdminSystem] Warning: Some schema offsets not resolved. Button detection may not work.\n");
    }
}

bool InitEntitySystem()
{
    if (!g_pGameResourceServiceServer)
    {
        META_CONPRINTF("[AdminSystem] Warning: IGameResourceService not available.\n");
    }

    // Load gamedata for engine-internal offsets (not available via schema)
    try
    {
        std::filesystem::path gamedata_path = std::filesystem::path(g_SMAPI->GetBaseDir())
            / "addons/admin-system/gamedata/admin_system.games.json";
        std::ifstream file(gamedata_path);
        if (!file.is_open())
        {
            META_CONPRINTF("[AdminSystem] Warning: gamedata/admin_system.games.json not found.\n");
            return true;
        }

        nlohmann::json gamedata = nlohmann::json::parse(file);

        if (gamedata.contains("offsets"))
        {
            auto& offsets = gamedata["offsets"];

#ifdef _WIN32
            const char* platform = "windows";
#else
            const char* platform = "linux";
#endif

            if (offsets.contains("GameEntitySystem") &&
                offsets["GameEntitySystem"].contains(platform))
            {
                s_offsetGameEntitySystem = offsets["GameEntitySystem"][platform].get<int>();
            }
        }

        META_CONPRINTF("[AdminSystem] Gamedata loaded (entity system offset: %d).\n",
                       s_offsetGameEntitySystem);
    }
    catch (const std::exception& e)
    {
        META_CONPRINTF("[AdminSystem] Warning: Failed to parse gamedata: %s\n", e.what());
    }

    // Resolve entity system pointer from IGameResourceService + offset
    if (g_pGameResourceServiceServer && s_offsetGameEntitySystem >= 0)
    {
        g_pEntitySystem = *reinterpret_cast<CGameEntitySystem**>(
            reinterpret_cast<uintptr_t>(g_pGameResourceServiceServer) + s_offsetGameEntitySystem);
    }

    if (g_pEntitySystem)
    {
        META_CONPRINTF("[AdminSystem] Entity system initialized.\n");
    }
    else
    {
        META_CONPRINTF("[AdminSystem] Warning: Entity system not available. Menu button detection disabled.\n");
    }

    return true;
}

CGameEntitySystem* GetEntitySystem()
{
    // Lazy re-resolve if not yet available (entity system may not exist until map load)
    if (!g_pEntitySystem && g_pGameResourceServiceServer && s_offsetGameEntitySystem >= 0)
    {
        g_pEntitySystem = *reinterpret_cast<CGameEntitySystem**>(
            reinterpret_cast<uintptr_t>(g_pGameResourceServiceServer) + s_offsetGameEntitySystem);
    }
    return g_pEntitySystem;
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
    if (!pSys || slot < 0 || slot >= MAXPLAYERS)
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
    if (!s_schemaOffsetsResolved)
        ResolveSchemaOffsets();

    if (s_offsetPlayerPawn < 0 || s_offsetMovementServices < 0 ||
        s_offsetButtons < 0 || s_offsetButtonStates < 0)
        return 0;

    // Step 1: Get player controller entity
    CEntityInstance* pController = GetPlayerController(slot);
    if (!pController)
        return 0;

    auto* pCtrlBase = reinterpret_cast<uint8_t*>(pController);

    // Step 2: Read m_hPlayerPawn handle (CHandle<CCSPlayerPawn> = uint32)
    uint32_t hPawn = *reinterpret_cast<uint32_t*>(pCtrlBase + s_offsetPlayerPawn);
    CEntityInstance* pPawn = ResolveEntityHandle(hPawn);
    if (!pPawn)
        return 0;

    auto* pPawnBase = reinterpret_cast<uint8_t*>(pPawn);

    // Step 3: Read m_pMovementServices pointer
    auto* pMovementServices = *reinterpret_cast<uint8_t**>(pPawnBase + s_offsetMovementServices);
    if (!pMovementServices)
        return 0;

    // Step 4: Read m_pButtonStates from CInButtonState (embedded at m_nButtons offset)
    // CInButtonState::m_pButtonStates is an array of 3 uint64:
    //   [0] = currently held buttons
    //   [1] = buttons that changed this frame
    //   [2] = scroll/other
    auto* pButtonStates = reinterpret_cast<uint64_t*>(
        pMovementServices + s_offsetButtons + s_offsetButtonStates);

    return pButtonStates[0]; // Current held buttons
}

bool IsPlayerSlotValid(int slot)
{
    return GetPlayerController(slot) != nullptr;
}

} // namespace sdk
