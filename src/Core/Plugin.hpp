#pragma once

#include <ISmmPlugin.h>

#include <eiface.h>
#include <icvar.h>

constexpr const char* ADMIN_SYSTEM_VERSION = "1.0.0";
constexpr const char* ADMIN_SYSTEM_AUTHOR = "m9snoi";
constexpr const char* ADMIN_SYSTEM_DESCRIPTION = "Admin System for CS2";
constexpr const char* ADMIN_SYSTEM_URL = "https://github.com/m9snoi/admin-system";

/**
 * Main Metamod:Source plugin entry point.
 * Handles plugin lifecycle (Load/Unload), SourceHook callbacks for game events
 * (GameFrame, client connect/disconnect, chat commands), and subsystem initialization.
 */
class AdminSystemPlugin : public ISmmPlugin, public IMetamodListener
{
public:
    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) override;
    bool Unload(char* error, size_t maxlen) override;
    bool Pause(char* error, size_t maxlen) override;
    bool Unpause(char* error, size_t maxlen) override;
    void AllPluginsLoaded() override;

    const char* GetAuthor() override;
    const char* GetName() override;
    const char* GetDescription() override;
    const char* GetURL() override;
    const char* GetLicense() override;
    const char* GetVersion() override;
    const char* GetDate() override;
    const char* GetLogTag() override;

    void* OnMetamodQuery(const char* iface, int* ret) override;

    void Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick);
    void Hook_OnClientConnected(CPlayerSlot slot, const char* pszName, uint64 xuid, const char* pszNetworkID,
                                const char* pszAddress, bool bFakePlayer);
    void Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char* pszName, uint64 xuid,
                               const char* pszNetworkID);
    void Hook_DispatchConCommand(ConCommandRef cmd, const CCommandContext& ctx, const CCommand& args);

    bool IsLateLoad() const { return _lateLoad; }

private:
    void RegisterHooks();
    void UnregisterHooks();

    bool _lateLoad = false;
};

extern AdminSystemPlugin g_AdminSystemPlugin;

PLUGIN_GLOBALVARS();
