#include "Plugin.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Admin/AdminMenu.hpp"
#include "../Players/PlayerManager.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "Adapters.hpp"
#include "Config.hpp"

#include "../../vendor/cs2-kit/src/Commands/Command.hpp"
#include "../../vendor/cs2-kit/src/Commands/CommandManager.hpp"
#include "../../vendor/cs2-kit/src/Core/ILogger.hpp"
#include "../../vendor/cs2-kit/src/Core/IPathResolver.hpp"
#include "../../vendor/cs2-kit/src/Core/Scheduler.hpp"
#include "../../vendor/cs2-kit/src/Menu/MenuManager.hpp"
#include "../../vendor/cs2-kit/src/Sdk/ConVarService.hpp"
#include "../../vendor/cs2-kit/src/Sdk/Entity.hpp"
#include "../../vendor/cs2-kit/src/Sdk/GameData.hpp"
#include "../../vendor/cs2-kit/src/Sdk/GameEventService.hpp"
#include "../../vendor/cs2-kit/src/Sdk/GameInterfaces.hpp"
#include "../../vendor/cs2-kit/src/Sdk/Schema.hpp"
#include "../../vendor/cs2-kit/src/Sdk/UserMessage.hpp"
#include "../../vendor/cs2-kit/src/Utils/Log.hpp"
#include "../../vendor/cs2-kit/src/Utils/Translations.hpp"

#include "../Database/Database.hpp"

#include <cstdio>
#include <cstring>
#include <engine/igameeventsystem.h>
#include <filesystem>
#include <icvar.h>
#include <interfaces/interfaces.h>
#include <networksystem/inetworkmessages.h>
#include <schemasystem/schemasystem.h>

using namespace AdminSystem::Admin;
using namespace AdminSystem::Core;
using namespace AdminSystem::Database;
using namespace AdminSystem::Players;
using namespace AdminSystem::Punishments;
using namespace CS2Kit::Commands;
using namespace CS2Kit::Menu;
using namespace CS2Kit::Sdk;
using namespace CS2Kit::Utils;

// Global plugin instance
AdminSystemPlugin g_AdminSystemPlugin;

// Adapter instances (file-scope, lifetime matches the plugin)
static HL2SDKLogger g_logger;
static PluginPathResolver g_pathResolver;
static SdkMenuIO g_menuIO;

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

    // Initialize CS2Kit adapters
    g_pathResolver.SetBaseDir(ismm->GetBaseDir());
    CS2Kit::Core::SetGlobalLogger(&g_logger);
    CS2Kit::Core::SetGlobalPathResolver(&g_pathResolver);

    Log::Info("Loading v{}...", ADMIN_SYSTEM_VERSION);

    // Get SDK interfaces - store in centralized GameInterfaces singleton
    auto& gi = GameInterfaces::Instance();

    GET_V_IFACE_ANY(GetServerFactory, gi.ServerGameDLL, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
    GET_V_IFACE_ANY(GetServerFactory, gi.ServerGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
    GET_V_IFACE_ANY(GetEngineFactory, gi.GameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, gi.NetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, gi.SchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, gi.GameResourceService, IGameResourceService,
                        GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, gi.CVar, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, gi.Engine, IVEngineServer2, INTERFACEVERSION_VENGINESERVER);

    // Also set the SDK global g_pCVar — required by ConCommandRef::GetName()
    // (convar.cpp from HL2SDK uses this global internally)
    g_pCVar = gi.CVar;

    // Initialize subsystems
    if (!InitializeSubsystems(late))
    {
        snprintf(error, maxlen, "Failed to initialize subsystems");
        return false;
    }

    // Register hooks
    RegisterHooks();

    Log::Info("Loaded successfully{}.", late ? " (late)" : "");
    return true;
}

bool AdminSystemPlugin::Unload(char* error, size_t maxlen)
{
    Log::Info("Unloading...");

    UnregisterHooks();
    ShutdownSubsystems();

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

void AdminSystemPlugin::AllPluginsLoaded()
{
    // Called after all plugins have loaded
    // Good place to check for optional dependencies
}

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
    CS2Kit::Core::Scheduler::Instance().OnGameFrame();
    MenuManager::Instance().OnGameFrame();
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

    // Check if player is admin
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

    // Clean up menu state
    MenuManager::Instance().OnPlayerDisconnect(playerSlot);

    // Remove player
    PlayerManager::Instance().RemovePlayer(playerSlot);
}

void AdminSystemPlugin::Hook_DispatchConCommand(ConCommandRef cmd, const CCommandContext& ctx, const CCommand& args)
{
    const char* cmdName = cmd.GetName();
    if (!cmdName)
        return;

    // Only intercept say and say_team
    bool isSay = (strcmp(cmdName, "say") == 0);
    bool isSayTeam = (strcmp(cmdName, "say_team") == 0);

    if (!isSay && !isSayTeam)
        return;

    if (args.ArgC() < 2)
        return;

    std::string message = args.Arg(1);

    // Strip surrounding quotes if present
    if (message.size() >= 2 && message.front() == '"' && message.back() == '"')
    {
        message = message.substr(1, message.size() - 2);
    }

    // Only process commands (check configured prefixes)
    if (message.empty())
        return;

    // Get player slot from context
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

    // 2. Load game data (signatures and offsets)
    Log::Info("Loading game data...");
    GameData::Instance().Load("addons/admin-system/gamedata/signatures.jsonc");

    // 3. Initialize SDK message system
    Log::Info("Initializing SDK message system...");
    if (!MessageSystem::Instance().Initialize())
    {
        Log::Error("Failed to initialize message system.");
        return false;
    }

    // 4. Initialize schema system (resolves entity field offsets at runtime)
    Log::Info("Initializing schema system...");
    if (!SchemaService::Instance().Initialize())
    {
        Log::Warn("Schema system init failed (button detection may not work).");
    }

    // 5. Initialize entity system
    Log::Info("Initializing entity system...");
    if (!EntitySystem::Instance().Initialize())
    {
        Log::Warn("Entity system init failed (menus may not work).");
    }

    // 6. Resolve IGameEventManager2 via signature scanning
    Log::Info("Resolving game event manager...");
    if (!MessageSystem::Instance().InitGameEventManager())
    {
        Log::Warn("Game event manager not resolved (center HTML display will not work).");
    }

    // 6a. Initialize ConVar service
    Log::Info("Initializing ConVar service...");
    if (!ConVarService::Instance().Initialize())
    {
        Log::Warn("ConVar service init failed.");
    }

    // 6b. Initialize game event service
    Log::Info("Initializing game event service...");
    if (!GameEventService::Instance().Initialize())
    {
        Log::Warn("Game event service init failed.");
    }

    // 6c. Initialize menu I/O adapter
    MenuManager::Instance().SetMenuIO(&g_menuIO);

    // 6d. Set up command permission callback
    CommandManager::Instance().SetPermissionCallback(
        [](int64_t steamId, const std::string& permission) -> bool {
            return AdminManager::Instance().HasAnyPermission(steamId, permission);
        });

    // 7. Initialize database connection (optional -- plugin works without DB)
    Log::Info("Connecting to database...");
    auto& db = Database::Instance();
    auto& configMgr = ConfigManager::Instance();

    bool dbConnected = db.Initialize(configMgr.GetDatabaseConfig());
    if (!dbConnected)
    {
        Log::Warn("Database not available. Running without DB features.");
    }

    // 8. Load admin groups and admins
    {
        auto& adminMgr = AdminManager::Instance();

        // 8a. Load from database first (if connected)
        if (dbConnected)
        {
            Log::Info("Loading admins from database...");

            if (!adminMgr.LoadGroups())
                Log::Warn("Failed to load admin groups from DB.");

            if (!adminMgr.LoadAdmins())
                Log::Warn("Failed to load admins from DB.");
        }

        // 8b. Load from JSON config (groups + admins in one file)
        Log::Info("Loading admins from config files...");
        if (!configMgr.LoadAdminsConfig("addons/admin-system/configs/admins.json"))
            Log::Warn("Failed to load admins.json");
    }

    // 9. Initialize player manager (singleton, auto-initialized)
    Log::Info("Player manager ready.");

    // 10. Initialize command manager and register commands
    Log::Info("Initializing commands...");
    auto& cmdMgr = CommandManager::Instance();

    // Register !admin command using CommandBuilder
    cmdMgr.Register(CommandBuilder("admin")
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

    // 11. Initialize punishment manager (requires DB)
    if (dbConnected)
    {
        Log::Info("Loading active punishments...");
        auto& punishmentMgr = PunishmentManager::Instance();
        if (!punishmentMgr.LoadActivePunishments())
            Log::Warn("Failed to load active punishments.");
    }

    // 12. Load translations
    Log::Info("Loading translations...");
    auto& translations = CS2Kit::Utils::Translations::Instance();
    translations.Load("addons/admin-system/configs/translations");

    // 13. Initialize menu manager (singleton, ready to use)
    Log::Info("Menu manager ready.");

    Log::Info("All subsystems initialized.");
    return true;
}

void AdminSystemPlugin::ShutdownSubsystems()
{
    Log::Info("Shutting down subsystems...");

    // Cancel all scheduled timers
    CS2Kit::Core::Scheduler::Instance().CancelAll();

    // Clear player manager
    PlayerManager::Instance().Clear();

    // Shutdown database
    Database::Instance().Shutdown();

    Log::Info("Subsystems shut down.");
}

bool AdminSystemPlugin::LoadConfigs()
{
    auto& configMgr = ConfigManager::Instance();

    // Load consolidated settings (plugin, database, commands, punishments, admin)
    if (!configMgr.LoadSettings("addons/admin-system/configs/settings.json"))
        Log::Warn("Failed to load settings.json");

    return true;
}

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
