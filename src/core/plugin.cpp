#include "plugin.h"

#include "../admin/admin_manager.h"
#include "../commands/command_manager.h"
#include "../database/database.h"
#include "../player/player_manager.h"
#include "../punishments/punishment_manager.h"
#include "config.h"

#include <cstdio>

// Global plugin instance
AdminSystemPlugin g_AdminSystemPlugin;

// Metamod plugin expose
PLUGIN_EXPOSE(AdminSystemPlugin, g_AdminSystemPlugin);

//-----------------------------------------------------------------------------
// ISmmPlugin Interface Implementation
//-----------------------------------------------------------------------------

bool AdminSystemPlugin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS();
    m_lateLoad = late;

    // Log plugin loading
    META_CONPRINTF("[Admin System] Loading v%s...\n", ADMIN_SYSTEM_VERSION);

    // Initialize subsystems
    if (!InitializeSubsystems(late))
    {
        snprintf(error, maxlen, "Failed to initialize subsystems");
        return false;
    }

    // Register hooks
    RegisterHooks();

    META_CONPRINTF("[Admin System] Loaded successfully%s.\n", late ? " (late)" : "");
    return true;
}

bool AdminSystemPlugin::Unload(char* error, size_t maxlen)
{
    META_CONPRINTF("[Admin System] Unloading...\n");

    // Unregister hooks
    UnregisterHooks();

    // Shutdown subsystems
    ShutdownSubsystems();

    META_CONPRINTF("[Admin System] Unloaded.\n");
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
// Private Methods
//-----------------------------------------------------------------------------

bool AdminSystemPlugin::InitializeSubsystems(bool late)
{
    // 1. Load configuration files
    META_CONPRINTF("[Admin System] Loading configurations...\n");
    if (!LoadConfigs())
    {
        META_CONPRINTF("[Admin System] Warning: Failed to load some configs.\n");
        return false;
    }

    // 2. Initialize database connection
    META_CONPRINTF("[Admin System] Connecting to database...\n");
    auto& db = database::Database::Instance();
    auto& config_mgr = core::ConfigManager::Instance();

    if (!db.Initialize(config_mgr.GetDatabaseConfig()))
    {
        META_CONPRINTF("[Admin System] Error: Failed to connect to database.\n");
        return false;
    }

    // 3. Load admin groups and admins
    META_CONPRINTF("[Admin System] Loading admins...\n");
    auto& admin_mgr = admin::AdminManager::Instance();

    if (!admin_mgr.LoadGroups())
    {
        META_CONPRINTF("[Admin System] Warning: Failed to load admin groups.\n");
    }

    if (!admin_mgr.LoadAdmins())
    {
        META_CONPRINTF("[Admin System] Warning: Failed to load admins.\n");
    }

    // 4. Initialize player manager (singleton, auto-initialized)
    META_CONPRINTF("[Admin System] Player manager ready.\n");

    // 5. Initialize command manager
    META_CONPRINTF("[Admin System] Initializing commands...\n");
    auto& cmd_mgr = commands::CommandManager::Instance();
    cmd_mgr.InitializeBuiltinCommands();

    // 6. Initialize punishment manager
    META_CONPRINTF("[Admin System] Loading active punishments...\n");
    auto& punishment_mgr = punishments::PunishmentManager::Instance();
    if (!punishment_mgr.LoadActivePunishments())
    {
        META_CONPRINTF("[Admin System] Warning: Failed to load active punishments.\n");
    }

    // 7. TODO: Initialize menu manager

    META_CONPRINTF("[Admin System] All subsystems initialized.\n");
    return true;
}

void AdminSystemPlugin::ShutdownSubsystems()
{
    META_CONPRINTF("[Admin System] Shutting down subsystems...\n");

    // Clear player manager
    player::PlayerManager::Instance().Clear();

    // Shutdown database
    database::Database::Instance().Shutdown();

    META_CONPRINTF("[Admin System] Subsystems shut down.\n");
}

bool AdminSystemPlugin::LoadConfigs()
{
    auto& config_mgr = core::ConfigManager::Instance();

    // Load main config
    if (!config_mgr.LoadConfig("addons/admin-system/configs/config.json"))
    {
        META_CONPRINTF("[Admin System] Warning: Failed to load config.json\n");
    }

    // Load database config
    if (!config_mgr.LoadDatabaseConfig("addons/admin-system/configs/database.json"))
    {
        META_CONPRINTF("[Admin System] Error: Failed to load database.json\n");
        return false;
    }

    // Load admins and groups (will be loaded into AdminManager after DB init)
    if (!config_mgr.LoadAdmins("addons/admin-system/configs/admins.json"))
    {
        META_CONPRINTF("[Admin System] Warning: Failed to load admins.json\n");
    }

    if (!config_mgr.LoadGroups("addons/admin-system/configs/groups.json"))
    {
        META_CONPRINTF("[Admin System] Warning: Failed to load groups.json\n");
    }

    return true;
}

void AdminSystemPlugin::RegisterHooks()
{
    // TODO: Register SourceHook hooks for:
    // - Client connect/disconnect
    // - Client command
    // - Voice/chat events
}

void AdminSystemPlugin::UnregisterHooks()
{
    // TODO: Unregister all hooks
}
