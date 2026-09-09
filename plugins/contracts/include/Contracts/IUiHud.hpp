#pragma once

#include <string_view>

namespace Contracts
{

/** The fixed HUD cards the ui plugin draws, in the order they are stacked on screen. */
enum class HudCard : int
{
    Server = 0,
    Wheel,
    Drop,
    Count,
};

/** Colour of a card's edge or a toast's stripe. The order is checked against the ui plugin's
 *  screen (`Cs2Ui::Hud::AccentNames`, from `panorama/screens/cs2_hud.css`) at compile time in
 *  Hud.cpp, so the two must be reordered together. No theme file names these any more. */
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
    /** 0..20 fills the bar; -1 hides it. */
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
    static constexpr std::string_view InterfaceName = "cs2plugins.IUiHud/1";

    /** Draw @p view on @p card. @p slot -1 means everyone. */
    virtual void SetCard(HudCard card, int slot, const CardView& view) = 0;

    /** Take @p card off screen for @p slot, -1 being everyone. */
    virtual void HideCard(HudCard card, int slot) = 0;

    /** Show a toast to @p slot, -1 being everyone, replacing whatever it is already showing. */
    virtual void Toast(int slot, const ToastView& view) = 0;

protected:
    // Consumers borrow; they never own or delete.
    ~IUiHud() = default;
};

}  // namespace Contracts
