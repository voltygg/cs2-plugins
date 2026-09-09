#include "Hud.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <VoltMod/Ui/UiPanel.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <string>

using Contracts::CardView;
using Contracts::HudCard;
using Contracts::ToastView;

namespace Ui
{

/** Contracts::HudAccent, lowercase and in enumerator order: what the screen's Accent classes
 *  must spell. A reorder on either side fails this at compile time. */
static constexpr std::array<std::string_view, 11> ExpectedAccentNames{
    "success", "warning", "info",      "error",     "common",    "uncommon",
    "rare",    "mythical", "legendary", "ancient",  "contraband",
};

static constexpr bool AccentNamesMatch()
{
    if (Cs2Ui::Hud::AccentNames.size() != ExpectedAccentNames.size())
        return false;
    for (std::size_t i = 0; i < ExpectedAccentNames.size(); ++i)
        if (Cs2Ui::Hud::AccentNames[i] != ExpectedAccentNames[i])
            return false;
    return true;
}
static_assert(AccentNamesMatch(), "cs2_hud's Accent classes must match Contracts::HudAccent");

Hud::Hud(VoltMod::Runtime& runtime, const ConfigManager& config) : _rt(runtime), _config(config)
{
    _toastTimers.BindReset(_rt.Slots);
}

void Hud::Start()
{
    // A client only has the layout once it has finished downloading the addon; drawing before
    // that writes into a panel it cannot render yet.
    _subs.Add(_rt.Addons.Ready += [this](int slot) {
        _screen.Show(slot);
        DrawServerCard();
    });

    _subs.Add(_rt.Players.FullyConnected += [this](VoltMod::Player&) { DrawServerCard(); });
    _subs.Add(_rt.Players.Disconnected += [this](VoltMod::Player&) { DrawServerCard(); });

    DrawServerCard();
}

void Hud::Publish()
{
    _rt.Exchange.Publish<Contracts::IUiHud>(this);
}

void Hud::Unpublish()
{
    _rt.Exchange.Unpublish<Contracts::IUiHud>();
}

bool Hud::Show(int slot)
{
    if (slot == VoltMod::UiPanel::Everyone)
    {
        if (!_shownEveryone)
            _shownEveryone = _screen.Show(slot);
        return _shownEveryone;
    }
    if (!VoltMod::IsValidSlot(slot))
        return false;
    return _screen.Shown(slot) || _screen.Show(slot);
}

void Hud::SetCard(HudCard card, int slot, const CardView& view)
{
    const int index = static_cast<int>(card);
    if (index < 0 || index >= static_cast<int>(HudCard::Count) || !Show(slot))
        return;

    const auto& row = Cs2Ui::Hud::Card[index];
    VoltMod::UiPanel& panel = _screen.Panel();
    row.Title.Write(panel, slot, view.Title);
    row.Subtitle.Write(panel, slot, view.Subtitle);
    row.Value.Write(panel, slot, view.Value);
    row.Icon.Write(panel, slot, view.Icon.empty() ? -1 : row.Icon.Find(view.Icon));
    // The card block has no separate bar flag, so an empty bar is step 0.
    row.Bar.Write(panel, slot, std::clamp(view.BarStep, 0, row.Bar.Count - 1));
    row.Accent.Write(panel, slot, static_cast<int>(view.Accent));
    row.Hidden.Write(panel, slot, false);
}

void Hud::HideCard(HudCard card, int slot)
{
    const int index = static_cast<int>(card);
    if (index < 0 || index >= static_cast<int>(HudCard::Count) || !Show(slot))
        return;
    Cs2Ui::Hud::Card[index].Hidden.Write(_screen.Panel(), slot, true);
}

void Hud::Toast(int slot, const ToastView& view)
{
    if (!Show(slot))
        return;

    VoltMod::UiPanel& panel = _screen.Panel();
    Cs2Ui::Hud::Toast.Title.Write(panel, slot, view.Title);
    Cs2Ui::Hud::Toast.Description.Write(panel, slot, view.Description);
    Cs2Ui::Hud::Toast.Accent.Write(panel, slot, static_cast<int>(view.Accent));
    Cs2Ui::Hud::Toast.Show.Write(panel, slot, true);

    const int duration = view.DurationMs > 0 ? view.DurationMs : _config.Get().ui.toastDurationMs;
    ToastTimer(slot) = _rt.Scheduler.Delay(
        duration, [this, slot] { Cs2Ui::Hud::Toast.Show.Write(_screen.Panel(), slot, false); });
}

VoltMod::Subscription& Hud::ToastTimer(int slot)
{
    return VoltMod::IsValidSlot(slot) ? _toastTimers[slot] : _everyoneToast;
}

void Hud::DrawServerCard()
{
    int online = 0;
    for (const VoltMod::Player* player : _rt.Players.All())
        if (!player->IsBot())
            ++online;

    const std::string subtitle =
        _rt.Translations.Get("hud.online", std::map<std::string, std::string>{{"count", std::to_string(online)}});

    SetCard(HudCard::Server, VoltMod::UiPanel::Everyone,
            {.Title = _config.Get().ui.serverName, .Subtitle = subtitle, .Accent = Contracts::HudAccent::Info});
}

}  // namespace Ui
