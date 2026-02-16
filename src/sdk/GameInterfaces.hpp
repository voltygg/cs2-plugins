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
