#pragma once

#include "App.hpp"

#include <CS2Kit/Api.hpp>
#include <optional>
#include <string_view>

/**
 * Admin System plugin. CS2Kit::MetamodPlugin owns the Metamod lifecycle, standard hooks and
 * player tracking; this class provides the metadata, owns the plugin's object graph for one
 * load cycle, and adds the one custom hook (voice-mute listening).
 */
class AdminSystemPlugin final : public CS2Kit::MetamodPlugin
{
protected:
    CS2Kit::PluginInfo Info() const override;
    bool OnLoad(CS2Kit::Runtime& runtime, bool late) override;
    void OnUnload() override { _app.reset(); }
    void OnPlayerConnect(CS2Kit::Player* player) override;
    void OnPlayerDisconnect(CS2Kit::Player* player) override;
    bool OnPlayerChat(CS2Kit::Player* player, std::string_view message, bool teamChat) override;
    void OnRegisterHooks(CS2Kit::Runtime& runtime) override;

public:
    // Engine asks per (receiver, sender) whether the receiver should hear the sender; we drop
    // the channel when the sender is voice-muted.
    bool Hook_SetClientListening(CPlayerSlot iReceiver, CPlayerSlot iSender, bool bListen);

private:
    std::optional<AdminSystem::App> _app;
    CS2Kit::Subscription _clientListening;
};
