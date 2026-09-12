#pragma once

#include "Config.hpp"
#include "Ui/Cs2Hud.hpp"

#include <Contracts/IUiHud.hpp>
#include <VoltMod/Api.hpp>
#include <VoltMod/Core/PerSlot.hpp>
#include <VoltMod/Core/Subscriptions.hpp>
#include <VoltMod/Ui/Screen.hpp>
#include <VoltMod/Ui/UiPanel.hpp>
#include <VoltMod/Ui/Writers.hpp>

namespace Ui
{

/**
 * The server HUD: one shared Panorama screen, three cards and a toast, published to other
 * plugins as Contracts::IUiHud.
 *
 * Every string a caller passes is written through before the call returns, so nothing is
 * retained. A player who has not downloaded the addon simply renders nothing; the writes still
 * succeed.
 */
class Hud final : public Contracts::IUiHud
{
public:
    Hud(VoltMod::Runtime& runtime, const ConfigManager& config);

    /** Subscribe to the roster and the addon feed, and start drawing the server card. */
    void Start();

    void Publish();
    /** Called before the runtime services this holds go away. */
    void Unpublish();

    bool SetCard(Contracts::HudCard card, int slot, const Contracts::CardView& view) override;
    bool HideCard(Contracts::HudCard card, int slot) override;
    void Toast(int slot, const Contracts::ToastView& view) override;

private:
    /** Ask for a Server card redraw on the next tick. A map change readies the whole roster one
     *  slot at a time, and one redraw covers all of them. */
    void DrawServerCard();

    /** Title and live player count of the Server card, for everyone. */
    void WriteServerCard();

    /** The one-shot that hides @p slot's toast; starting it again replaces the running one. */
    VoltMod::Subscription& ToastTimer(int slot);

    VoltMod::Runtime& _rt;
    const ConfigManager& _config;
    VoltMod::Screen _screen{_rt.Ui, Cs2Ui::Hud::Layout, Cs2Ui::Hud::RootId};
    VoltMod::Subscription _serverCardRedraw;
    VoltMod::PerSlot<VoltMod::Subscription> _toastTimers;
    VoltMod::Subscription _everyoneToast;
    VoltMod::Subscriptions _subs;
};

}  // namespace Ui
