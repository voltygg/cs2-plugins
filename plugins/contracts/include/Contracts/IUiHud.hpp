#pragma once

#include <string_view>

namespace Contracts
{

/** The slot value meaning everyone: what is drawn for it is seen by every player. Spelled here
 *  rather than taken from VoltMod::EveryoneSlot so contracts stays dependency-free. */
inline constexpr int Everyone = -1;

/** Which of the cards the HUD stacks top-left a write lands on. */
enum class HudCard : int
{
    First,
    Second,
    Third,
};

/** How many cards @ref HudCard names, for a screen that has to carry one panel per card. */
inline constexpr int HudCardCount = 3;

/** The card the ui plugin draws itself: the server name over a live player count. A plugin takes
 *  one of the others, and two plugins on the same card overwrite each other. */
inline constexpr HudCard ServerCard = HudCard::First;

/** Colour of a card's edge or a toast's stripe. The order is checked against the ui plugin's
 *  screen (`Cs2Ui::Hud::AccentNames`, from `panorama/screens/cs2_hud.css`) at compile time in
 *  Hud.cpp, so the two must be reordered together. */
enum class HudAccent : int
{
    None = -1,
    Success,
    Warning,
    Info,
    Error,
    Common,
    Uncommon,
    Rare,
    Mythical,
    Legendary,
    Ancient,
    Contraband,
};

/** What one card shows. Empty text hides that line. */
struct CardView
{
    std::string_view Title;
    std::string_view Subtitle;
    std::string_view Value;
    /** Icon-set name, e.g. "ump45"; empty draws no icon. */
    std::string_view Icon;
    /** 0..20 fills the bar; a negative value hides it. */
    int BarStep = -1;
    HudAccent Accent = HudAccent::None;
};

/** A transient notice that fades itself out. */
struct ToastView
{
    std::string_view Title;
    std::string_view Description;
    HudAccent Accent = HudAccent::None;
    /** 0 uses the ui plugin's `toastDurationMs` setting. */
    int DurationMs = 0;
};

/**
 * @brief The server HUD offered to other plugins through VoltMod's ServiceExchange.
 *
 * Published by the ui plugin in OnLoad. Callers get nullptr from the exchange when it is not
 * loaded, and nothing is drawn for a player who has not downloaded the workshop addon.
 *
 * Each plugin has its own operator new, so nothing crosses that owns memory: every string is a
 * view copied before the call returns. Any change to this vtable or to what a parameter means
 * bumps the /N in InterfaceName.
 */
struct IUiHud
{
    static constexpr std::string_view InterfaceName = "cs2plugins.IUiHud/3";

    /** Draw @p view on @p card for @p slot, or @ref Everyone. False when nothing could be drawn:
     *  a card the screen does not have, or a screen that could not be spawned. */
    virtual bool SetCard(HudCard card, int slot, const CardView& view) = 0;

    /** Take @p card off screen for @p slot, or @ref Everyone. False for the same reasons. */
    virtual bool HideCard(HudCard card, int slot) = 0;

    /** Show a toast to @p slot, or @ref Everyone, replacing whatever it is already showing. */
    virtual void Toast(int slot, const ToastView& view) = 0;

protected:
    // Consumers borrow; they never own or delete.
    ~IUiHud() = default;
};

}  // namespace Contracts
