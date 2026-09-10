#include "Hud.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <VoltMod/Ui/UiPanel.hpp>
#include <VoltMod/Ui/Writers.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>

using Contracts::CardView;
using Contracts::HudCard;
using Contracts::ToastView;

namespace Ui
{

namespace Screen = Cs2Ui::Hud;

// Contracts::HudAccent, lowercase and in enumerator order. A reorder on either side fails here.
static_assert(Screen::AccentNames == std::array<std::string_view, 11>{"success", "warning", "info", "error", "common",
                                                                     "uncommon", "rare", "mythical", "legendary",
                                                                     "ancient", "contraband"},
              "cs2_hud's Accent classes must match Contracts::HudAccent");

/** One card of the screen: which panel each writer touches, and which variable it reads. */
struct CardWriters
{
    VoltMod::TextVar Title;
    VoltMod::TextVar Subtitle;
    VoltMod::TextVar Value;
    VoltMod::ClassChoice Icon;
    VoltMod::ClassChoice Bar;
    VoltMod::ClassChoice Accent;
    VoltMod::ClassFlag Hidden;
};

static constexpr CardWriters MakeCard(const Screen::Card& card)
{
    return {
        .Title = {Screen::RootId, card.TitleVar},
        .Subtitle = {Screen::RootId, card.SubtitleVar},
        .Value = {Screen::RootId, card.ValueVar},
        .Icon = {card.Icon, Screen::IconClasses},
        .Bar = {card.Bar, Screen::StepClasses},
        .Accent = {card.Accent, Screen::AccentClasses},
        .Hidden = {card.Id, "Hidden"},
    };
}

static constexpr auto Cards = VoltMod::MakeWriters(Screen::Cards, MakeCard);
static_assert(Cards.size() == static_cast<std::size_t>(HudCard::Count), "cs2_hud ships one card per HudCard");

/** The toast. Named apart from Hud::Toast so a call site inside it needs no qualification. */
struct ToastWriters
{
    VoltMod::TextVar Title;
    VoltMod::TextVar Description;
    VoltMod::ClassChoice Accent;
    VoltMod::ClassFlag Show;
};

static constexpr ToastWriters ToastPanel{
    .Title = {Screen::RootId, Screen::ToastTitleVar},
    .Description = {Screen::RootId, Screen::ToastDescriptionVar},
    .Accent = {Screen::ToastAccent, Screen::AccentClasses},
    .Show = {Screen::Toast, "Show"},
};

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

void Hud::SetCard(HudCard card, int slot, const CardView& view)
{
    const int index = static_cast<int>(card);
    if (index < 0 || index >= static_cast<int>(HudCard::Count) || !_screen.Show(slot))
        return;

    const CardWriters& row = Cards[index];
    VoltMod::UiPanel& panel = _screen.Panel();
    row.Title.Write(panel, slot, view.Title);
    row.Subtitle.Write(panel, slot, view.Subtitle);
    row.Value.Write(panel, slot, view.Value);
    row.Icon.Write(panel, slot, view.Icon.empty() ? VoltMod::ClassChoice::None : row.Icon.Find(view.Icon));
    // The card block has no separate bar flag, so an empty bar is step 0.
    row.Bar.Write(panel, slot, std::clamp(view.BarStep, 0, row.Bar.Count() - 1));
    row.Accent.Write(panel, slot, static_cast<int>(view.Accent));
    row.Hidden.Write(panel, slot, false);
}

void Hud::HideCard(HudCard card, int slot)
{
    const int index = static_cast<int>(card);
    if (index < 0 || index >= static_cast<int>(HudCard::Count) || !_screen.Show(slot))
        return;
    Cards[index].Hidden.Write(_screen.Panel(), slot, true);
}

void Hud::Toast(int slot, const ToastView& view)
{
    if (!_screen.Show(slot))
        return;

    VoltMod::UiPanel& panel = _screen.Panel();
    ToastPanel.Title.Write(panel, slot, view.Title);
    ToastPanel.Description.Write(panel, slot, view.Description);
    ToastPanel.Accent.Write(panel, slot, static_cast<int>(view.Accent));
    ToastPanel.Show.Write(panel, slot, true);

    const int duration = view.DurationMs > 0 ? view.DurationMs : _config.Get().ui.toastDurationMs;
    ToastTimer(slot) =
        _rt.Scheduler.Delay(duration, [this, slot] { ToastPanel.Show.Write(_screen.Panel(), slot, false); });
}

VoltMod::Subscription& Hud::ToastTimer(int slot)
{
    return VoltMod::IsValidSlot(slot) ? _toastTimers[slot] : _everyoneToast;
}

void Hud::DrawServerCard()
{
    // Re-arming replaces the pending one, so a whole roster readying at once redraws once.
    _serverCardRedraw = _rt.Scheduler.NextTick([this] { WriteServerCard(); });
}

void Hud::WriteServerCard()
{
    int online = 0;
    for (const VoltMod::Player* player : _rt.Players.All())
        if (!player->IsBot())
            ++online;

    const std::string subtitle =
        _rt.Translations.Get("hud.online", std::map<std::string, std::string>{{"count", std::to_string(online)}});

    SetCard(HudCard::Server, VoltMod::EveryoneSlot,
            {.Title = _config.Get().ui.serverName, .Subtitle = subtitle, .Accent = Contracts::HudAccent::Info});
}

}  // namespace Ui
