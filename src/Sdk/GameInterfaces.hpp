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

namespace AdminSystem::Sdk
{

/**
 * Centralized holder for all HL2SDK interface pointers.
 * Populated during plugin Load() via Metamod's GET_V_IFACE macros.
 */
struct GameInterfaces : Core::Singleton<GameInterfaces>
{
    explicit GameInterfaces(Token) {}

    IServerGameDLL* ServerGameDLL = nullptr;
    IServerGameClients* ServerGameClients = nullptr;
    IVEngineServer2* Engine = nullptr;
    IGameEventSystem* GameEventSystem = nullptr;
    INetworkMessages* NetworkMessages = nullptr;
    IGameEventManager2* GameEventManager = nullptr;
    ISchemaSystem* SchemaSystem = nullptr;
    CGameEntitySystem* EntitySystem = nullptr;
    ICvar* CVar = nullptr;
    IGameResourceService* GameResourceService = nullptr;
};

}  // namespace AdminSystem::Sdk
