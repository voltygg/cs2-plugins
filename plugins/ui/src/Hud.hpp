#pragma once

#include "Config.hpp"
#include "Ui/Cs2Hud.hpp"

#include <Contracts/IUiHud.hpp>
#include <VoltMod/Api.hpp>
#include <VoltMod/Core/PerSlot.hpp>
#include <VoltMod/Core/SubscriptionScope.hpp>
#include <VoltMod/Ui/Screen.hpp>

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

    void SetCard(Contracts::HudCard card, int slot, const Contracts::CardView& view) override;
    void HideCard(Contracts::HudCard card, int slot) override;
    void Toast(int slot, const Contracts::ToastView& view) override;

private:
    /** Put the layout on @p slot's screen if it is not up yet. False when it cannot be. */
    bool Show(int slot);

    /** Title and live player count of the Server card, for everyone. */
    void DrawServerCard();

    /** The one-shot that hides @p slot's toast; re-arming it replaces the running one. */
    VoltMod::Subscription& ToastTimer(int slot);

    VoltMod::Runtime& _rt;
    const ConfigManager& _config;
    VoltMod::Screen _screen{_rt.Ui, _rt.Slots, Cs2Ui::Hud::Layout, Cs2Ui::Hud::RootId};
    /** Screen::Shown only tracks real slots, so the global state needs its own flag. */
    bool _shownEveryone = false;
    VoltMod::PerSlot<VoltMod::Subscription> _toastTimers;
    VoltMod::Subscription _everyoneToast;
    VoltMod::SubscriptionScope _subs;
};

}  // namespace Ui
