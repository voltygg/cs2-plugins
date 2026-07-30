#pragma once

#include "Managers.hpp"

#include <CS2Kit/Api.hpp>
#include <string_view>

/**
 * Admin System plugin. The Metamod lifecycle, standard hooks, player tracking, and the
 * Managers container lifetime are owned by CS2Kit::PluginBase; this class provides plugin
 * metadata, subsystem wiring (OnLoad), the gameplay callbacks, and the one custom hook
 * (voice-mute listening). Reach the managers via AdminSystem::App().
 */
class AdminSystemPlugin : public CS2Kit::PluginBase<AdminSystem::Managers>
{
public:
    ~AdminSystemPlugin();

    /** The single plugin instance. */
    static AdminSystemPlugin& Get();

protected:
    CS2Kit::PluginInfo Info() const override;
    bool OnLoad(bool late) override;
    void OnPlayerConnect(CS2Kit::Player* player) override;
    void OnPlayerDisconnect(CS2Kit::Player* player) override;
    bool OnPlayerChat(CS2Kit::Player* player, std::string_view message, bool teamChat) override;
    void OnRegisterHooks() override;

public:
    // Engine asks per (receiver, sender) whether the receiver should hear the sender; we drop
    // the channel when the sender is voice-muted.
    bool Hook_SetClientListening(CPlayerSlot iReceiver, CPlayerSlot iSender, bool bListen);
};

extern AdminSystemPlugin g_AdminSystemPlugin;
