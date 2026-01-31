#pragma once

// Metamod:Source 2.0 plugin header
// META_IS_SOURCE2 is defined by AMBuildScript for Source 2 games
#include <ISmmPlugin.h>

// Plugin version info
#define ADMIN_SYSTEM_VERSION "1.0.0"
#define ADMIN_SYSTEM_AUTHOR "m9snoi"
#define ADMIN_SYSTEM_DESCRIPTION "Admin System for CS2"
#define ADMIN_SYSTEM_URL "https://github.com/m9snoi/admin-system"

/**
 * @brief Main plugin class implementing ISmmPlugin interface
 *
 * This is the entry point for the Metamod:Source plugin.
 * It handles plugin lifecycle (Load, Unload, Pause, Unpause)
 * and initializes all subsystems.
 */
class AdminSystemPlugin : public ISmmPlugin, public IMetamodListener
{
public:
    // ISmmPlugin interface
    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) override;
    bool Unload(char* error, size_t maxlen) override;
    bool Pause(char* error, size_t maxlen) override;
    bool Unpause(char* error, size_t maxlen) override;
    void AllPluginsLoaded() override;

    // Plugin info
    const char* GetAuthor() override;
    const char* GetName() override;
    const char* GetDescription() override;
    const char* GetURL() override;
    const char* GetLicense() override;
    const char* GetVersion() override;
    const char* GetDate() override;
    const char* GetLogTag() override;

    // IMetamodListener
    void* OnMetamodQuery(const char* iface, int* ret) override;

public:
    // Accessors
    bool IsLateLoad() const { return m_lateLoad; }

private:
    bool InitializeSubsystems(bool late);
    void ShutdownSubsystems();
    bool LoadConfigs();

    void RegisterHooks();
    void UnregisterHooks();

private:
    bool m_lateLoad = false;
};

// Global plugin instance
extern AdminSystemPlugin g_AdminSystemPlugin;

// Metamod plugin globals macro
PLUGIN_GLOBALVARS();
