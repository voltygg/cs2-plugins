#pragma once

#include "../Core/Singleton.hpp"

#include <eiface.h>
#include <icvar.h>
#include <interfaces/interfaces.h>

// Forward declarations from HL2SDK (types not in standard includes)
class CGameEntitySystem;
class IGameEventSystem;
class INetworkMessages;
class IGameEventManager2;
class ISchemaSystem;

namespace AdminSystem::Sdk {

/**
 * Centralized holder for all HL2SDK interface pointers.
 * Populated during plugin Load() via Metamod's GET_V_IFACE macros.
 */
struct GameInterfaces : Core::Singleton<GameInterfaces> {
    friend class Core::Singleton<GameInterfaces>;

    IServerGameDLL* ServerGameDLL = nullptr;
    IServerGameClients* ServerGameClients = nullptr;
    IGameEventSystem* GameEventSystem = nullptr;
    INetworkMessages* NetworkMessages = nullptr;
    IGameEventManager2* GameEventManager = nullptr;
    ISchemaSystem* SchemaSystem = nullptr;
    CGameEntitySystem* EntitySystem = nullptr;
    ICvar* CVar = nullptr;
    IGameResourceService* GameResourceService = nullptr;

private:
    GameInterfaces() = default;
};

} // namespace AdminSystem::Sdk
