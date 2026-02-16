#include "Plugin.hpp"
#include "Config.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Admin/AdminMenus.hpp"
#include "../Commands/Command.hpp"
#include "../Commands/CommandManager.hpp"
#include "../Database/Database.hpp"
#include "../Menu/MenuManager.hpp"
#include "../Players/PlayerManager.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "../Sdk/Entity.hpp"
#include "../Sdk/GameData.hpp"
#include "../Sdk/GameInterfaces.hpp"
#include "../Sdk/Schema.hpp"
#include "../Sdk/UserMessage.hpp"
#include "../Utils/Translations.hpp"

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
    _lateLoad = late;

    META_CONPRINTF("[AdminSystem] Loading v%s...\n", ADMIN_SYSTEM_VERSION);

    // Get SDK interfaces - store in centralized GameInterfaces singleton
    auto& gi = AdminSystem::Sdk::GameInterfaces::Instance();

    GET_V_IFACE_ANY(GetServerFactory, gi.ServerGameDLL, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
    GET_V_IFACE_ANY(GetServerFactory, gi.ServerGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
    GET_V_IFACE_ANY(GetEngineFactory, gi.GameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, gi.NetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, gi.SchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, gi.GameResourceService, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, gi.CVar, ICvar, CVAR_INTERFACE_VERSION);

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

    UnregisterHooks();
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

const char* AdminSystemPlugin::GetAuthor()       { return ADMIN_SYSTEM_AUTHOR; }
const char* AdminSystemPlugin::GetName()         { return "Admin System"; }
const char* AdminSystemPlugin::GetDescription()  { return ADMIN_SYSTEM_DESCRIPTION; }
const char* AdminSystemPlugin::GetURL()          { return ADMIN_SYSTEM_URL; }
const char* AdminSystemPlugin::GetLicense()      { return "MIT"; }
const char* AdminSystemPlugin::GetVersion()      { return ADMIN_SYSTEM_VERSION; }
const char* AdminSystemPlugin::GetDate()         { return __DATE__; }
const char* AdminSystemPlugin::GetLogTag()       { return "ADMIN"; }

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
    AdminSystem::Menu::MenuManager::Instance().OnGameFrame();
}

void AdminSystemPlugin::Hook_OnClientConnected(CPlayerSlot slot, const char* pszName, uint64 xuid,
                                                const char* pszNetworkID, const char* pszAddress, bool bFakePlayer)
{
    if (bFakePlayer)
        return;

    int playerSlot = slot.Get();
    int64_t steamId = static_cast<int64_t>(xuid);

    auto& plrMgr = AdminSystem::Players::PlayerManager::Instance();
    plrMgr.AddPlayer(playerSlot, steamId, pszName ? pszName : "", pszAddress ? pszAddress : "");

    // Check if player is admin
    auto& adminMgr = AdminSystem::Admin::AdminManager::Instance();
    auto* player = plrMgr.GetPlayerBySlot(playerSlot);
    if (player)
    {
        player->SetAdmin(adminMgr.IsAdmin(steamId));
    }
}

void AdminSystemPlugin::Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason,
                                               const char* pszName, uint64 xuid, const char* pszNetworkID)
{
    int playerSlot = slot.Get();

    // Clean up menu state
    AdminSystem::Menu::MenuManager::Instance().OnPlayerDisconnect(playerSlot);

    // Remove player
    AdminSystem::Players::PlayerManager::Instance().RemovePlayer(playerSlot);
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

    // Quick check: at least one prefix must match
    auto& cmdMgr = AdminSystem::Commands::CommandManager::Instance();
    // Delegate prefix checking to HandleChatMessage

    // Get player slot from context
    int playerSlot = ctx.GetPlayerSlot().Get();
    if (playerSlot < 0 || playerSlot >= 64)
        return;

    auto* player = AdminSystem::Players::PlayerManager::Instance().GetPlayerBySlot(playerSlot);
    if (!player)
        return;

    bool handled = cmdMgr.HandleChatMessage(player, message);

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

    // 2. Load game data (signatures and offsets)
    META_CONPRINTF("[AdminSystem] Loading game data...\n");
    AdminSystem::Sdk::GameData::Instance().Load("addons/admin-system/gamedata");

    // 3. Initialize SDK message system
    META_CONPRINTF("[AdminSystem] Initializing SDK message system...\n");
    if (!AdminSystem::Sdk::InitMessageSystem())
    {
        META_CONPRINTF("[AdminSystem] Error: Failed to initialize message system.\n");
        return false;
    }

    // 4. Initialize schema system (resolves entity field offsets at runtime)
    META_CONPRINTF("[AdminSystem] Initializing schema system...\n");
    if (!AdminSystem::Sdk::InitSchemaSystem())
    {
        META_CONPRINTF("[AdminSystem] Warning: Schema system init failed (button detection may not work).\n");
    }

    // 5. Initialize entity system
    META_CONPRINTF("[AdminSystem] Initializing entity system...\n");
    if (!AdminSystem::Sdk::InitEntitySystem())
    {
        META_CONPRINTF("[AdminSystem] Warning: Entity system init failed (menus may not work).\n");
    }

    // 6. Resolve IGameEventManager2 via signature scanning
    META_CONPRINTF("[AdminSystem] Resolving game event manager...\n");
    if (!AdminSystem::Sdk::InitGameEventManager())
    {
        META_CONPRINTF("[AdminSystem] Warning: Game event manager not resolved (center HTML display will not work).\n");
    }

    // 7. Initialize database connection (optional -- plugin works without DB)
    META_CONPRINTF("[AdminSystem] Connecting to database...\n");
    auto& db = AdminSystem::Database::Database::Instance();
    auto& configMgr = AdminSystem::Core::ConfigManager::Instance();

    bool dbConnected = db.Initialize(configMgr.GetDatabaseConfig());
    if (!dbConnected)
    {
        META_CONPRINTF("[AdminSystem] Warning: Database not available. Running without DB features.\n");
    }

    // 8. Load admin groups and admins
    {
        auto& adminMgr = AdminSystem::Admin::AdminManager::Instance();

        // 8a. Load from database first (if connected)
        if (dbConnected)
        {
            META_CONPRINTF("[AdminSystem] Loading admins from database...\n");

            if (!adminMgr.LoadGroups())
                META_CONPRINTF("[AdminSystem] Warning: Failed to load admin groups from DB.\n");

            if (!adminMgr.LoadAdmins())
                META_CONPRINTF("[AdminSystem] Warning: Failed to load admins from DB.\n");
        }

        // 8b. Load from JSON config (groups + admins in one file)
        META_CONPRINTF("[AdminSystem] Loading admins from config files...\n");
        if (!configMgr.LoadAdminsConfig("addons/admin-system/configs/admins.json"))
            META_CONPRINTF("[AdminSystem] Warning: Failed to load admins.json\n");
    }

    // 9. Initialize player manager (singleton, auto-initialized)
    META_CONPRINTF("[AdminSystem] Player manager ready.\n");

    // 10. Initialize command manager and register commands
    META_CONPRINTF("[AdminSystem] Initializing commands...\n");
    auto& cmdMgr = AdminSystem::Commands::CommandManager::Instance();

    // Register !admin command using CommandBuilder
    cmdMgr.Register(
        AdminSystem::Commands::CommandBuilder("admin")
            .WithAliases({"a", "menu"})
            .WithDescription("Open the admin menu")
            .WithUsage("!admin")
            .RequirePermission("a")
            .WithArgs(0, 0)
            .OnExecute([](AdminSystem::Players::Player* admin,
                          const std::vector<std::string>& /*args*/) -> AdminSystem::Commands::CommandResult
            {
                META_CONPRINTF("[AdminSystem] !admin handler: slot=%d, steamid=%lld\n",
                               admin->GetSlot(), static_cast<long long>(admin->GetSteamID()));
                auto mainMenu = AdminSystem::Admin::BuildAdminMainMenu(admin->GetSlot());
                if (mainMenu)
                {
                    AdminSystem::Menu::MenuManager::Instance().OpenMenu(admin->GetSlot(), mainMenu);
                    return {true, "Admin menu opened"};
                }
                META_CONPRINTF("[AdminSystem] !admin handler: BuildAdminMainMenu returned nullptr!\n");
                return {false, "Failed to open admin menu"};
            })
            .Build()
    );

    // 11. Initialize punishment manager (requires DB)
    if (dbConnected)
    {
        META_CONPRINTF("[AdminSystem] Loading active punishments...\n");
        auto& punishmentMgr = AdminSystem::Punishments::PunishmentManager::Instance();
        if (!punishmentMgr.LoadActivePunishments())
            META_CONPRINTF("[AdminSystem] Warning: Failed to load active punishments.\n");
    }

    // 12. Load translations
    META_CONPRINTF("[AdminSystem] Loading translations...\n");
    auto& translations = AdminSystem::Utils::Translations::Instance();
    translations.Load("addons/admin-system/configs/translations");

    // 13. Initialize menu manager (singleton, ready to use)
    META_CONPRINTF("[AdminSystem] Menu manager ready.\n");

    META_CONPRINTF("[AdminSystem] All subsystems initialized.\n");
    return true;
}

void AdminSystemPlugin::ShutdownSubsystems()
{
    META_CONPRINTF("[AdminSystem] Shutting down subsystems...\n");

    // Clear player manager
    AdminSystem::Players::PlayerManager::Instance().Clear();

    // Shutdown database
    AdminSystem::Database::Database::Instance().Shutdown();

    META_CONPRINTF("[AdminSystem] Subsystems shut down.\n");
}

bool AdminSystemPlugin::LoadConfigs()
{
    auto& configMgr = AdminSystem::Core::ConfigManager::Instance();

    // Load consolidated settings (plugin, database, commands, punishments, admin)
    if (!configMgr.LoadSettings("addons/admin-system/configs/settings.json"))
        META_CONPRINTF("[AdminSystem] Warning: Failed to load settings.json\n");

    return true;
}

void AdminSystemPlugin::RegisterHooks()
{
    auto& gi = AdminSystem::Sdk::GameInterfaces::Instance();

    SH_ADD_HOOK(IServerGameDLL, GameFrame, gi.ServerGameDLL,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_GameFrame), true);
    SH_ADD_HOOK(IServerGameClients, OnClientConnected, gi.ServerGameClients,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_OnClientConnected), false);
    SH_ADD_HOOK(IServerGameClients, ClientDisconnect, gi.ServerGameClients,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_ClientDisconnect), true);
    SH_ADD_HOOK(ICvar, DispatchConCommand, gi.CVar,
                SH_MEMBER(this, &AdminSystemPlugin::Hook_DispatchConCommand), false);

    META_CONPRINTF("[AdminSystem] Hooks registered.\n");
}

void AdminSystemPlugin::UnregisterHooks()
{
    auto& gi = AdminSystem::Sdk::GameInterfaces::Instance();

    SH_REMOVE_HOOK(IServerGameDLL, GameFrame, gi.ServerGameDLL,
                   SH_MEMBER(this, &AdminSystemPlugin::Hook_GameFrame), true);
    SH_REMOVE_HOOK(IServerGameClients, OnClientConnected, gi.ServerGameClients,
                   SH_MEMBER(this, &AdminSystemPlugin::Hook_OnClientConnected), false);
    SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, gi.ServerGameClients,
                   SH_MEMBER(this, &AdminSystemPlugin::Hook_ClientDisconnect), true);
    SH_REMOVE_HOOK(ICvar, DispatchConCommand, gi.CVar,
                   SH_MEMBER(this, &AdminSystemPlugin::Hook_DispatchConCommand), false);

    META_CONPRINTF("[AdminSystem] Hooks unregistered.\n");
}
