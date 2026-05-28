#pragma once

#include <CS2Kit/Core/MetamodPluginBase.hpp>
#include <string_view>

/**
 * Admin System plugin. The Metamod lifecycle, standard hooks, and player tracking are
 * owned by CS2Kit::Core::MetamodPluginBase; this class provides plugin metadata, subsystem
 * wiring (OnLoad), the gameplay callbacks, and the one custom hook (voice-mute listening).
 */
class AdminSystemPlugin : public CS2Kit::Core::MetamodPluginBase
{
protected:
    CS2Kit::Core::PluginInfo Info() const override;
    bool OnLoad(bool late) override;
    void OnPlayerConnect(CS2Kit::Players::Player* player) override;
    void OnPlayerDisconnect(CS2Kit::Players::Player* player) override;
    bool OnPlayerChat(CS2Kit::Players::Player* player, std::string_view message, bool teamChat) override;
    void OnRegisterHooks() override;

public:
    // Engine asks per (receiver, sender) whether the receiver should hear the sender; we drop
    // the channel when the sender is voice-muted.
    bool Hook_SetClientListening(CPlayerSlot iReceiver, CPlayerSlot iSender, bool bListen);
};

extern AdminSystemPlugin g_AdminSystemPlugin;

PLUGIN_GLOBALVARS();
