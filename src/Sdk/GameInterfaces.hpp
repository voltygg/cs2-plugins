#pragma once

#include "../Core/Singleton.hpp"

#include <eiface.h>
#include <icvar.h>
#include <interfaces/interfaces.h>

// Forward declarations for HL2SDK types not exposed by standard includes
class CGameEntitySystem;
class IGameEventSystem;
class INetworkMessages;
class IGameEventManager2;
class ISchemaSystem;

namespace AdminSystem::Sdk
{

/**
 * Centralized holder for all HL2SDK interface pointers.
 *
 * All fields are populated during Plugin::Load() via Metamod's
 * `GET_V_IFACE_ANY` / `GET_V_IFACE_CURRENT` macros. Subsystems access
 * interfaces through this singleton instead of scattered globals.
 *
 * @note GameEventManager is not obtained via GET_V_IFACE; it is resolved
 *       through signature scanning in MessageSystem::InitGameEventManager().
 * @note EntitySystem is resolved at an offset from GameResourceService
 *       in EntitySystem::Initialize().
 */
struct GameInterfaces : Core::Singleton<GameInterfaces>
{
    explicit GameInterfaces(Token) {}

    IServerGameDLL* ServerGameDLL = nullptr;              ///< Server game DLL interface (hooks GameFrame, etc.).
    IServerGameClients* ServerGameClients = nullptr;      ///< Client connection/disconnection hooks.
    IVEngineServer2* Engine = nullptr;                    ///< Engine services: kick, server commands, voice.
    IGameEventSystem* GameEventSystem = nullptr;          ///< Protobuf network message dispatch.
    INetworkMessages* NetworkMessages = nullptr;          ///< Network message registration and lookup.
    IGameEventManager2* GameEventManager = nullptr;       ///< Legacy game event system (resolved via sig scan).
    ISchemaSystem* SchemaSystem = nullptr;                ///< Runtime entity schema/offset resolution.
    CGameEntitySystem* EntitySystem = nullptr;            ///< Global entity system (resolved from GameResourceService).
    ICvar* CVar = nullptr;                                ///< ConVar system interface.
    IGameResourceService* GameResourceService = nullptr;  ///< Resource service (provides entity system pointer).
};

}  // namespace AdminSystem::Sdk
