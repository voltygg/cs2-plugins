#include "Plugin.hpp"

#include "../Admin/Effects/EffectManager.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "Bootstrap.hpp"
#include "ChatService.hpp"

#include <CS2Kit/CS2Kit.hpp>
#include <CS2Kit/Core/Scheduler.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/GameInterfaces.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <cstdio>
#include <cstring>

using namespace AdminSystem::Core;
using namespace AdminSystem::Admin::Effects;
using namespace AdminSystem::Punishments;
using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;
using namespace CS2Kit::Core;
using namespace CS2Kit::Sdk;

// Global plugin instance
AdminSystemPlugin g_AdminSystemPlugin;

// Metamod plugin expose
PLUGIN_EXPOSE(AdminSystemPlugin, g_AdminSystemPlugin);

// SourceHook hook declarations (file scope)
SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK6_void(IServerGameClients, OnClientConnected, SH_NOATTRIB, 0, CPlayerSlot, const char*, uint64, const char*,
                   const char*, bool);
SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, ENetworkDisconnectionReason,
                   const char*, uint64, const char*);
SH_DECL_HOOK3_void(ICvar, DispatchConCommand, SH_NOATTRIB, 0, ConCommandRef, const CCommandContext&, const CCommand&);

// ------- ISmmPlugin Interface Implementation -----

bool AdminSystemPlugin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS();
    _lateLoad = late;

    CS2Kit::InitParams params;
    params.LogPrefix = "AdminSystem";

    if (!CS2Kit::Initialize(ismm, error, maxlen, params))
        return false;

    Log::Info("Loading v{}...", ADMIN_SYSTEM_VERSION);

    if (!Bootstrap::Initialize())
    {
        snprintf(error, maxlen, "Failed to initialize subsystems");
        return false;
    }

    RegisterHooks();
    Log::Info("Loaded successfully{}.", late ? " (late)" : "");
    return true;
}

bool AdminSystemPlugin::Unload(char* error, size_t maxlen)
{
    Log::Info("Unloading...");

    UnregisterHooks();
    Bootstrap::Shutdown();
    CS2Kit::Shutdown();

    Log::Info("Unloaded.");
    return true;
}

bool AdminSystemPlugin::Pause(char* error, size_t maxlen)
{
    return true;
}

bool AdminSystemPlugin::Unpause(char* error, size_t maxlen)
{
    return true;
}

void AdminSystemPlugin::AllPluginsLoaded() {}

// ------- Plugin Info -----

const char* AdminSystemPlugin::GetAuthor()
{
    return ADMIN_SYSTEM_AUTHOR;
}
const char* AdminSystemPlugin::GetName()
{
    return "Admin System";
}
const char* AdminSystemPlugin::GetDescription()
{
    return ADMIN_SYSTEM_DESCRIPTION;
}
const char* AdminSystemPlugin::GetURL()
{
    return ADMIN_SYSTEM_URL;
}
const char* AdminSystemPlugin::GetLicense()
{
    return "MIT";
}
const char* AdminSystemPlugin::GetVersion()
{
    return ADMIN_SYSTEM_VERSION;
}
const char* AdminSystemPlugin::GetDate()
{
    return __DATE__;
}
const char* AdminSystemPlugin::GetLogTag()
{
    return "ADMIN";
}

// ------- IMetamodListener -----

void* AdminSystemPlugin::OnMetamodQuery(const char* iface, int* ret)
{
    if (ret)
        *ret = META_IFACE_FAILED;
    return nullptr;
}

// ------- SourceHook Callbacks -----

void AdminSystemPlugin::Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick)
{
    CS2Kit::OnGameFrame();
}

void AdminSystemPlugin::Hook_OnClientConnected(CPlayerSlot slot, const char* pszName, uint64 xuid,
                                               const char* pszNetworkID, const char* pszAddress, bool bFakePlayer)
{
    int playerSlot = slot.Get();
    int64_t steamId = static_cast<int64_t>(xuid);

    PlayerManager::Instance().AddPlayer(playerSlot, steamId, pszName ? pszName : "", pszAddress ? pszAddress : "");

    // Bots have no real SteamID, so they can't be banned/muted/gagged — but we still want them
    // in PlayerManager so they show up in the admin menu and are kickable for testing.
    if (bFakePlayer)
        return;

    // Reject banned players. Kicking inside the connect hook is unsafe in some builds, so we defer
    // to the next game frame via the scheduler — the player is fully connected by then.
    if (auto ban = PunishmentManager::Instance().GetActiveBan(steamId))
    {
        std::string reason = ban->Reason;
        Scheduler::Instance().NextTick([playerSlot, reason]() { PlayerController(playerSlot).Kick(reason.c_str()); });
    }
}

void AdminSystemPlugin::Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char* pszName,
                                              uint64 xuid, const char* pszNetworkID)
{
    int playerSlot = slot.Get();
    EffectManager::Instance().CancelAllForSlot(playerSlot);
    CS2Kit::OnPlayerDisconnect(playerSlot);
    PlayerManager::Instance().RemovePlayer(playerSlot);
}

void AdminSystemPlugin::Hook_DispatchConCommand(ConCommandRef cmd, const CCommandContext& ctx, const CCommand& args)
{
    const char* cmdName = cmd.GetName();
    if (!cmdName)
        return;

    bool isSay = (strcmp(cmdName, "say") == 0);
    bool isSayTeam = (strcmp(cmdName, "say_team") == 0);
    if (!isSay && !isSayTeam)
        return;

    if (args.ArgC() < 2)
        return;

    std::string message = args.Arg(1);
    if (message.size() >= 2 && message.front() == '"' && message.back() == '"')
        message = message.substr(1, message.size() - 2);
    if (message.empty())
        return;

    int playerSlot = ctx.GetPlayerSlot().Get();
    if (playerSlot < 0 || playerSlot >= 64)
        return;

    auto* player = PlayerManager::Instance().GetPlayerBySlot(playerSlot);
    if (!player)
        return;

    if (ChatService::Instance().HandleSay(player, message, isSayTeam))
        RETURN_META(MRES_SUPERCEDE);
}

// ------- Hook Registration -----

void AdminSystemPlugin::RegisterHooks()
{
    auto& gi = GameInterfaces::Instance();

    SH_ADD_HOOK(IServerGameDLL, GameFrame, gi.ServerGameDLL, SH_MEMBER(this, &AdminSystemPlugin::Hook_GameFrame), true);
    SH_ADD_HOOK(IServerGameClients, OnClientConnected, gi.ServerGameClients,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_OnClientConnected), false);
    SH_ADD_HOOK(IServerGameClients, ClientDisconnect, gi.ServerGameClients,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_ClientDisconnect), true);
    SH_ADD_HOOK(ICvar, DispatchConCommand, gi.CVar, SH_MEMBER(this, &AdminSystemPlugin::Hook_DispatchConCommand),
                false);

    Log::Info("Hooks registered.");
}

void AdminSystemPlugin::UnregisterHooks()
{
    auto& gi = GameInterfaces::Instance();

    SH_REMOVE_HOOK(IServerGameDLL, GameFrame, gi.ServerGameDLL, SH_MEMBER(this, &AdminSystemPlugin::Hook_GameFrame),
                   true);
    SH_REMOVE_HOOK(IServerGameClients, OnClientConnected, gi.ServerGameClients,
                   SH_MEMBER(this, &AdminSystemPlugin::Hook_OnClientConnected), false);
    SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, gi.ServerGameClients,
                   SH_MEMBER(this, &AdminSystemPlugin::Hook_ClientDisconnect), true);
    SH_REMOVE_HOOK(ICvar, DispatchConCommand, gi.CVar, SH_MEMBER(this, &AdminSystemPlugin::Hook_DispatchConCommand),
                   false);

    Log::Info("Hooks unregistered.");
}
