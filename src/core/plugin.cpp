#include "plugin.h"

#include "../admin/admin_manager.h"
#include "../commands/command_manager.h"
#include "../database/database.h"
#include "../menus/admin_menu.h"
#include "../menus/menu_manager.h"
#include "../player/player_manager.h"
#include "../punishments/punishment_manager.h"
#include "../sdk/entity.h"
#include "../sdk/schema.h"
#include "../sdk/usermessage.h"
#include "../utils/translations.h"
#include "config.h"

#include <engine/igameeventsystem.h>
#include <networksystem/inetworkmessages.h>
#include <schemasystem/schemasystem.h>
#include <interfaces/interfaces.h>
#include <icvar.h>

#include <cstdio>
#include <cstring>
#include <filesystem>

// Global plugin instance
AdminSystemPlugin g_AdminSystemPlugin;

// Global SDK interface pointers
IServerGameDLL* g_pServerGameDLL = nullptr;
IServerGameClients* g_pServerGameClients = nullptr;

// Metamod plugin expose
PLUGIN_EXPOSE(AdminSystemPlugin, g_AdminSystemPlugin);

// SourceHook hook declarations (file scope)
SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK6_void(IServerGameClients, OnClientConnected, SH_NOATTRIB, 0, CPlayerSlot, const char*, uint64, const char*, const char*, bool);
SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, ENetworkDisconnectionReason, const char*, uint64, const char*);
SH_DECL_HOOK3_void(ICvar, DispatchConCommand, SH_NOATTRIB, 0, ConCommandRef, const CCommandContext&, const CCommand&);

//-----------------------------------------------------------------------------
// ISmmPlugin Interface Implementation
//-----------------------------------------------------------------------------

bool AdminSystemPlugin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS();
    m_lateLoad = late;

    // Log plugin loading
    META_CONPRINTF("[AdminSystem] Loading v%s...\n", ADMIN_SYSTEM_VERSION);

    // Get SDK interfaces
    GET_V_IFACE_ANY(GetServerFactory, g_pServerGameDLL, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
    GET_V_IFACE_ANY(GetServerFactory, g_pServerGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
    GET_V_IFACE_ANY(GetEngineFactory, sdk::g_pGameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, sdk::g_pNetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, sdk::g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);

    // Initialize subsystems
    if (!InitializeSubsystems(late))
    {
        snprintf(error, maxlen, "Failed to initialize subsystems");
        return false;
    }

    // Register hooks
    RegisterHooks();

    META_CONPRINTF("[AdminSystem] Loaded successfully%s.\n", late ? " (late)" : "");
    return true;
}

bool AdminSystemPlugin::Unload(char* error, size_t maxlen)
{
    META_CONPRINTF("[AdminSystem] Unloading...\n");

    // Unregister hooks
    UnregisterHooks();

    // Shutdown subsystems
    ShutdownSubsystems();

    META_CONPRINTF("[AdminSystem] Unloaded.\n");
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
    menus::MenuManager::Instance().OnGameFrame();
}

void AdminSystemPlugin::Hook_OnClientConnected(CPlayerSlot slot, const char* pszName, uint64 xuid,
                                                const char* pszNetworkID, const char* pszAddress, bool bFakePlayer)
{
    if (bFakePlayer)
        return;

    int playerSlot = slot.Get();
    int64_t steamId = static_cast<int64_t>(xuid);

    auto& player_mgr = player::PlayerManager::Instance();
    player_mgr.AddPlayer(playerSlot, steamId, pszName ? pszName : "", pszAddress ? pszAddress : "");

    // Check if player is admin
    auto& admin_mgr = admin::AdminManager::Instance();
    auto* player = player_mgr.GetPlayerBySlot(playerSlot);
    if (player)
    {
        player->SetAdmin(admin_mgr.IsAdmin(steamId));
    }
}

void AdminSystemPlugin::Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason,
                                               const char* pszName, uint64 xuid, const char* pszNetworkID)
{
    int playerSlot = slot.Get();

    // Clean up menu state
    menus::MenuManager::Instance().OnPlayerDisconnect(playerSlot);

    // Remove player
    player::PlayerManager::Instance().RemovePlayer(playerSlot);
}

void AdminSystemPlugin::Hook_DispatchConCommand(ConCommandRef cmd, const CCommandContext& ctx, const CCommand& args)
{
    // Get command name
    const char* cmdName = cmd.GetName();
    if (!cmdName)
        return;

    // Only intercept say and say_team
    bool isSay = (strcmp(cmdName, "say") == 0);
    bool isSayTeam = (strcmp(cmdName, "say_team") == 0);

    if (!isSay && !isSayTeam)
        return;

    // Get the chat message text (arg 1)
    if (args.ArgC() < 2)
        return;

    std::string message = args.Arg(1);

    // Strip surrounding quotes if present
    if (message.size() >= 2 && message.front() == '"' && message.back() == '"')
    {
        message = message.substr(1, message.size() - 2);
    }

    // Only process commands (! or . prefix)
    if (message.empty() || (message[0] != '!' && message[0] != '.'))
        return;

    // Get player slot from context
    int playerSlot = ctx.GetPlayerSlot().Get();
    if (playerSlot < 0 || playerSlot >= 64)
        return;

    // Look up player
    auto* player = player::PlayerManager::Instance().GetPlayerBySlot(playerSlot);
    if (!player)
        return;

    // Dispatch to command manager
    bool handled = commands::CommandManager::Instance().HandleChatMessage(player, message);

    if (handled)
    {
        META_CONPRINTF("[AdminSystem] Command handled: %s (player slot %d)\n", message.c_str(), playerSlot);
    }
}

//-----------------------------------------------------------------------------
// Private Methods
//-----------------------------------------------------------------------------

bool AdminSystemPlugin::InitializeSubsystems(bool late)
{
    // 1. Load configuration files
    META_CONPRINTF("[AdminSystem] Loading configurations...\n");
    if (!LoadConfigs())
    {
        META_CONPRINTF("[AdminSystem] Warning: Failed to load some configs.\n");
        return false;
    }

    // 2. Initialize SDK message system
    META_CONPRINTF("[AdminSystem] Initializing SDK message system...\n");
    if (!sdk::InitMessageSystem())
    {
        META_CONPRINTF("[AdminSystem] Error: Failed to initialize message system.\n");
        return false;
    }

    // 3. Initialize schema system (resolves entity field offsets at runtime)
    META_CONPRINTF("[AdminSystem] Initializing schema system...\n");
    if (!sdk::InitSchemaSystem())
    {
        META_CONPRINTF("[AdminSystem] Warning: Schema system init failed (button detection may not work).\n");
    }

    // 4. Initialize entity system
    META_CONPRINTF("[AdminSystem] Initializing entity system...\n");
    if (!sdk::InitEntitySystem())
    {
        META_CONPRINTF("[AdminSystem] Warning: Entity system init failed (menus may not work).\n");
    }

    // 4b. Resolve IGameEventManager2 via signature scanning (requires gamedata loaded in step 4)
    META_CONPRINTF("[AdminSystem] Resolving game event manager...\n");
    if (!sdk::InitGameEventManager())
    {
        META_CONPRINTF("[AdminSystem] Warning: Game event manager not resolved (center HTML display will not work).\n");
    }

    // 5. Initialize database connection (optional — plugin works without DB)
    META_CONPRINTF("[AdminSystem] Connecting to database...\n");
    auto& db = database::Database::Instance();
    auto& config_mgr = core::ConfigManager::Instance();

    bool dbConnected = db.Initialize(config_mgr.GetDatabaseConfig());
    if (!dbConnected)
    {
        META_CONPRINTF("[AdminSystem] Warning: Database not available. Running without DB features.\n");
    }

    // 6. Load admin groups and admins
    {
        auto& admin_mgr = admin::AdminManager::Instance();

        // 6a. Load from database first (if connected)
        if (dbConnected)
        {
            META_CONPRINTF("[AdminSystem] Loading admins from database...\n");

            if (!admin_mgr.LoadGroups())
            {
                META_CONPRINTF("[AdminSystem] Warning: Failed to load admin groups from DB.\n");
            }

            if (!admin_mgr.LoadAdmins())
            {
                META_CONPRINTF("[AdminSystem] Warning: Failed to load admins from DB.\n");
            }
        }

        // 6b. Load from JSON files (supplements/overrides DB entries)
        // Done after DB load because admin_mgr.LoadAdmins() clears m_admins first.
        META_CONPRINTF("[AdminSystem] Loading admins from config files...\n");
        if (!config_mgr.LoadGroups("addons/admin-system/configs/groups.json"))
        {
            META_CONPRINTF("[AdminSystem] Warning: Failed to load groups.json\n");
        }
        if (!config_mgr.LoadAdmins("addons/admin-system/configs/admins.json"))
        {
            META_CONPRINTF("[AdminSystem] Warning: Failed to load admins.json\n");
        }
    }

    // 7. Initialize player manager (singleton, auto-initialized)
    META_CONPRINTF("[AdminSystem] Player manager ready.\n");

    // 8. Initialize command manager and register commands
    META_CONPRINTF("[AdminSystem] Initializing commands...\n");
    auto& cmd_mgr = commands::CommandManager::Instance();
    cmd_mgr.InitializeBuiltinCommands();

    // Register !admin command
    cmd_mgr.RegisterCommand({
        .name = "admin",
        .aliases = {"a", "menu"},
        .description = "Open the admin menu",
        .usage = "!admin",
        .permission = "a",
        .min_args = 0,
        .max_args = 0,
        .handler = [](player::Player* admin, const std::vector<std::string>& /*args*/) -> commands::CommandResult
        {
            META_CONPRINTF("[AdminSystem] !admin handler: slot=%d, steamid=%lld\n",
                           admin->GetSlot(), static_cast<long long>(admin->GetSteamID()));
            auto main_menu = menus::BuildAdminMainMenu(admin->GetSlot());
            if (main_menu)
            {
                menus::MenuManager::Instance().OpenMenu(admin->GetSlot(), main_menu);
                return {true, "Admin menu opened"};
            }
            META_CONPRINTF("[AdminSystem] !admin handler: BuildAdminMainMenu returned nullptr!\n");
            return {false, "Failed to open admin menu"};
        },
    });

    // 9. Initialize punishment manager (requires DB)
    if (dbConnected)
    {
        META_CONPRINTF("[AdminSystem] Loading active punishments...\n");
        auto& punishment_mgr = punishments::PunishmentManager::Instance();
        if (!punishment_mgr.LoadActivePunishments())
        {
            META_CONPRINTF("[AdminSystem] Warning: Failed to load active punishments.\n");
        }
    }

    // 10. Load translations
    META_CONPRINTF("[AdminSystem] Loading translations...\n");
    auto& translations = utils::Translations::Instance();
    translations.Load("addons/admin-system/configs/translations");

    // 11. Initialize menu manager (singleton, ready to use)
    META_CONPRINTF("[AdminSystem] Menu manager ready.\n");

    META_CONPRINTF("[AdminSystem] All subsystems initialized.\n");
    return true;
}

void AdminSystemPlugin::ShutdownSubsystems()
{
    META_CONPRINTF("[AdminSystem] Shutting down subsystems...\n");

    // Clear player manager
    player::PlayerManager::Instance().Clear();

    // Shutdown database
    database::Database::Instance().Shutdown();

    META_CONPRINTF("[AdminSystem] Subsystems shut down.\n");
}

bool AdminSystemPlugin::LoadConfigs()
{
    auto& config_mgr = core::ConfigManager::Instance();

    // Load main config
    if (!config_mgr.LoadConfig("addons/admin-system/configs/config.json"))
    {
        META_CONPRINTF("[AdminSystem] Warning: Failed to load config.json\n");
    }

    // Load database config (non-fatal — plugin works without DB)
    if (!config_mgr.LoadDatabaseConfig("addons/admin-system/configs/database.json"))
    {
        META_CONPRINTF("[AdminSystem] Warning: Failed to load database.json\n");
    }

    return true;
}

void AdminSystemPlugin::RegisterHooks()
{
    SH_ADD_HOOK(IServerGameDLL, GameFrame, g_pServerGameDLL,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_GameFrame), true);
    SH_ADD_HOOK(IServerGameClients, OnClientConnected, g_pServerGameClients,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_OnClientConnected), false);
    SH_ADD_HOOK(IServerGameClients, ClientDisconnect, g_pServerGameClients,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_ClientDisconnect), true);
    SH_ADD_HOOK(ICvar, DispatchConCommand, g_pCVar,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_DispatchConCommand), false);

    META_CONPRINTF("[AdminSystem] Hooks registered.\n");
}

void AdminSystemPlugin::UnregisterHooks()
{
    SH_REMOVE_HOOK(IServerGameDLL, GameFrame, g_pServerGameDLL,
                   SH_MEMBER(this, &AdminSystemPlugin::Hook_GameFrame), true);
    SH_REMOVE_HOOK(IServerGameClients, OnClientConnected, g_pServerGameClients,
                   SH_MEMBER(this, &AdminSystemPlugin::Hook_OnClientConnected), false);
    SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, g_pServerGameClients,
                   SH_MEMBER(this, &AdminSystemPlugin::Hook_ClientDisconnect), true);
    SH_REMOVE_HOOK(ICvar, DispatchConCommand, g_pCVar,
                   SH_MEMBER(this, &AdminSystemPlugin::Hook_DispatchConCommand), false);

    META_CONPRINTF("[AdminSystem] Hooks unregistered.\n");
}
