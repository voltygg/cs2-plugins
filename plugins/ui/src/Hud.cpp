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
using VoltMod::ClassChoice;

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

/** The writers for @p card, or null for a card the screen does not have. */
static const CardWriters* CardAt(HudCard card)
{
    const int index = static_cast<int>(card);
    if (index < 0 || index >= static_cast<int>(Cards.size()))
        return nullptr;
    return &Cards[static_cast<std::size_t>(index)];
}

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
    const CardWriters* writers = CardAt(card);
    if (!writers || !_screen.Show(slot))
        return;

    const PanelWriter w{_screen.Panel(), slot};
    w.Set(writers->Title, view.Title);
    w.Set(writers->Subtitle, view.Subtitle);
    w.Set(writers->Value, view.Value);
    w.Set(writers->Icon, view.Icon.empty() ? ClassChoice::None : writers->Icon.Find(view.Icon));
    // The card block has no separate bar flag, so an empty bar is step 0.
    w.Set(writers->Bar, std::clamp(view.BarStep, 0, writers->Bar.Count() - 1));
    w.Set(writers->Accent, static_cast<int>(view.Accent));
    w.Set(writers->Hidden, false);
}

void Hud::HideCard(HudCard card, int slot)
{
    const CardWriters* writers = CardAt(card);
    if (!writers || !_screen.Show(slot))
        return;

    PanelWriter{_screen.Panel(), slot}.Set(writers->Hidden, true);
}

void Hud::Toast(int slot, const ToastView& view)
{
    if (!_screen.Show(slot))
        return;

    const PanelWriter w{_screen.Panel(), slot};
    w.Set(ToastPanel.Title, view.Title);
    w.Set(ToastPanel.Description, view.Description);
    w.Set(ToastPanel.Accent, static_cast<int>(view.Accent));
    w.Set(ToastPanel.Show, true);

    const int duration = view.DurationMs > 0 ? view.DurationMs : _config.Get().ui.toastDurationMs;
    ToastTimer(slot) =
        _rt.Scheduler.Delay(duration, [this, slot] { PanelWriter{_screen.Panel(), slot}.Set(ToastPanel.Show, false); });
}

VoltMod::Subscription& Hud::ToastTimer(int slot)
{
    return VoltMod::IsValidSlot(slot) ? _toastTimers[slot] : _everyoneToast;
}

void Hud::DrawServerCard()
{
    // Starting the tick again replaces the one waiting, so a whole roster readying at once redraws once.
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
