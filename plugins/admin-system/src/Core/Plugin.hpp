#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/MetamodPluginBase.hpp>
#include <memory>
#include <string_view>

namespace AdminSystem
{
struct Managers;
}

/**
 * Admin System plugin. The Metamod lifecycle, standard hooks, and player tracking are
 * owned by CS2Kit::MetamodPluginBase; this class provides plugin metadata, subsystem
 * wiring (OnLoad), the gameplay callbacks, and the one custom hook (voice-mute listening).
 *
 * The plugin owns its service managers (AdminSystem::Managers) for one Load/Unload cycle, so
 * their state cannot leak across `meta unload`/`meta reload`. Reach them via AdminSystem::App().
 */
class AdminSystemPlugin : public CS2Kit::MetamodPluginBase
{
public:
    ~AdminSystemPlugin();

    /** The single plugin instance. */
    static AdminSystemPlugin& Get();

protected:
    CS2Kit::PluginInfo Info() const override;
    bool OnLoad(bool late) override;
    void OnDestroyInstances() override;
    void OnPlayerConnect(CS2Kit::Player* player) override;
    void OnPlayerDisconnect(CS2Kit::Player* player) override;
    bool OnPlayerChat(CS2Kit::Player* player, std::string_view message, bool teamChat) override;
    void OnRegisterHooks() override;

public:
    // Engine asks per (receiver, sender) whether the receiver should hear the sender; we drop
    // the channel when the sender is voice-muted.
    bool Hook_SetClientListening(CPlayerSlot iReceiver, CPlayerSlot iSender, bool bListen);

private:
    std::unique_ptr<AdminSystem::Managers> _managers;
};

extern AdminSystemPlugin g_AdminSystemPlugin;

PLUGIN_GLOBALVARS();
