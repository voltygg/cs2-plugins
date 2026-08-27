#pragma once

#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <optional>
#include <string_view>

/**
 * Admin System plugin. VoltMod::MetamodPlugin owns the Metamod lifecycle, standard hooks and
 * player tracking; this class provides the metadata, owns the plugin's object graph for one
 * load cycle, and adds the one custom hook (voice-mute listening).
 */
class AdminSystemPlugin final : public VoltMod::MetamodPlugin
{
protected:
    VoltMod::PluginInfo Info() const override;
    bool OnLoad(VoltMod::Runtime& runtime, bool late) override;
    void OnUnload() override { _app.reset(); }
    bool OnPlayerChat(VoltMod::Player* player, std::string_view message, bool teamChat) override;
    void OnRegisterHooks(VoltMod::Runtime& runtime) override;

public:
    // Engine asks per (receiver, sender) whether the receiver should hear the sender; we drop
    // the channel when the sender is voice-muted.
    bool Hook_SetClientListening(CPlayerSlot iReceiver, CPlayerSlot iSender, bool bListen);

private:
    std::optional<AdminSystem::App> _app;
};
