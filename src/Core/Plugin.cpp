#include "Plugin.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Admin/AdminMenu.hpp"
#include "../Database/Database.hpp"
#include "../Players/PlayerManager.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "Config.hpp"
#include "PlayerCaller.hpp"

#include <CS2Kit/CS2Kit.hpp>
#include <CS2Kit/Commands/Command.hpp>
#include <CS2Kit/Commands/CommandManager.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Sdk/GameInterfaces.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <cstdio>
#include <cstring>

using namespace AdminSystem::Admin;
using namespace AdminSystem::Core;
using namespace AdminSystem::Database;
using namespace AdminSystem::Players;
using namespace AdminSystem::Punishments;
using namespace CS2Kit::Commands;
using namespace CS2Kit::Menu;
using namespace CS2Kit::Utils;

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

//-----------------------------------------------------------------------------
// ISmmPlugin Interface Implementation
//-----------------------------------------------------------------------------

bool AdminSystemPlugin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS();
    _lateLoad = late;

    // Initialize CS2Kit (resolves SDK interfaces, loads gamedata, inits subsystems)
    CS2Kit::InitParams params;
    params.LogPrefix = "AdminSystem";

    if (!CS2Kit::Initialize(ismm, error, maxlen, params))
        return false;

    Log::Info("Loading v{}...", ADMIN_SYSTEM_VERSION);

    // Initialize admin-system-specific subsystems
    if (!InitializeSubsystems(late))
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
    ShutdownSubsystems();
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

//-----------------------------------------------------------------------------
// Plugin Info
//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------
// IMetamodListener
//-----------------------------------------------------------------------------

void* AdminSystemPlugin::OnMetamodQuery(const char* iface, int* ret)
{
    if (ret)
        *ret = META_IFACE_FAILED;
    return nullptr;
}

//-----------------------------------------------------------------------------
// SourceHook Callbacks
//-----------------------------------------------------------------------------

void AdminSystemPlugin::Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick)
{
    CS2Kit::OnGameFrame();
}

void AdminSystemPlugin::Hook_OnClientConnected(CPlayerSlot slot, const char* pszName, uint64 xuid,
                                               const char* pszNetworkID, const char* pszAddress, bool bFakePlayer)
{
    if (bFakePlayer)
        return;

    int playerSlot = slot.Get();
    int64_t steamId = static_cast<int64_t>(xuid);

    auto& plrMgr = PlayerManager::Instance();
    plrMgr.AddPlayer(playerSlot, steamId, pszName ? pszName : "", pszAddress ? pszAddress : "");

    auto& adminMgr = AdminManager::Instance();
    auto* player = plrMgr.GetPlayerBySlot(playerSlot);
    if (player)
    {
        player->SetAdmin(adminMgr.IsAdmin(steamId));
    }
}

void AdminSystemPlugin::Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char* pszName,
                                              uint64 xuid, const char* pszNetworkID)
{
    int playerSlot = slot.Get();
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
    {
        message = message.substr(1, message.size() - 2);
    }

    if (message.empty())
        return;

    int playerSlot = ctx.GetPlayerSlot().Get();
    if (playerSlot < 0 || playerSlot >= 64)
        return;

    auto* player = PlayerManager::Instance().GetPlayerBySlot(playerSlot);
    if (!player)
        return;

    PlayerCaller caller(player);
    auto& cmdMgr = CommandManager::Instance();
    bool handled = cmdMgr.HandleChatMessage(&caller, message);

    if (handled)
    {
        Log::Info("Command handled: {} (player slot {})", message, playerSlot);
    }
}

//-----------------------------------------------------------------------------
// Private Methods
//-----------------------------------------------------------------------------

bool AdminSystemPlugin::InitializeSubsystems(bool late)
{
    // 1. Load configuration files
    Log::Info("Loading configurations...");
    if (!LoadConfigs())
    {
        Log::Warn("Failed to load some configs.");
        return false;
    }

    // 2. Set up command permission callback
    CommandManager::Instance().SetPermissionCallback([](int64_t steamId, const std::string& permission) -> bool {
        return AdminManager::Instance().HasAnyPermission(steamId, permission);
    });

    // 3. Initialize database connection (optional)
    Log::Info("Connecting to database...");
    auto& db = Database::Instance();
    auto& configMgr = ConfigManager::Instance();

    bool dbConnected = db.Initialize(configMgr.GetDatabaseConfig());
    if (!dbConnected)
    {
        Log::Warn("Database not available. Running without DB features.");
    }

    // 4. Load admin groups and admins
    {
        auto& adminMgr = AdminManager::Instance();

        if (dbConnected)
        {
            Log::Info("Loading admins from database...");
            if (!adminMgr.LoadGroups())
                Log::Warn("Failed to load admin groups from DB.");
            if (!adminMgr.LoadAdmins())
                Log::Warn("Failed to load admins from DB.");
        }

        Log::Info("Loading admins from config files...");
        if (!configMgr.LoadAdminsConfig("addons/admin-system/configs/admins.json"))
            Log::Warn("Failed to load admins.json");
    }

    Log::Info("Player manager ready.");

    // 5. Register commands
    Log::Info("Initializing commands...");
    auto& cmdMgr = CommandManager::Instance();

    cmdMgr.Register(
        CommandBuilder("admin")
            .WithAliases({"a", "menu"})
            .WithDescription("Open the admin menu")
            .WithUsage("!admin")
            .RequirePermission("a")
            .WithArgs(0, 0)
            .OnExecute([](ICommandCaller* caller, const std::vector<std::string>& /*args*/) -> CommandResult {
                auto* playerCaller = dynamic_cast<PlayerCaller*>(caller);
                if (!playerCaller || !playerCaller->GetPlayer())
                    return {false, "Invalid caller"};

                auto* admin = playerCaller->GetPlayer();
                Log::Info("!admin handler: slot={}, steamid={}", admin->GetSlot(), admin->GetSteamID());
                auto mainMenu = BuildAdminMainMenu(admin->GetSlot());
                if (mainMenu)
                {
                    MenuManager::Instance().OpenMenu(admin->GetSlot(), mainMenu);
                    return {true, "Admin menu opened"};
                }
                Log::Error("!admin handler: BuildAdminMainMenu returned nullptr!");
                return {false, "Failed to open admin menu"};
            })
            .Build());

    // 6. Initialize punishment manager (requires DB)
    if (dbConnected)
    {
        Log::Info("Loading active punishments...");
        auto& punishmentMgr = PunishmentManager::Instance();
        if (!punishmentMgr.LoadActivePunishments())
            Log::Warn("Failed to load active punishments.");
    }

    // 7. Load translations
    Log::Info("Loading translations...");
    auto& translations = Translations::Instance();
    translations.Load("addons/admin-system/configs/translations");

    Log::Info("All subsystems initialized.");
    return true;
}

void AdminSystemPlugin::ShutdownSubsystems()
{
    Log::Info("Shutting down subsystems...");
    PlayerManager::Instance().Clear();
    Database::Instance().Shutdown();
    Log::Info("Subsystems shut down.");
}

bool AdminSystemPlugin::LoadConfigs()
{
    auto& configMgr = ConfigManager::Instance();
    if (!configMgr.LoadSettings("addons/admin-system/configs/settings.json"))
        Log::Warn("Failed to load settings.json");
    return true;
}

void AdminSystemPlugin::RegisterHooks()
{
    auto& gi = CS2Kit::Sdk::GameInterfaces::Instance();

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
    auto& gi = CS2Kit::Sdk::GameInterfaces::Instance();

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
