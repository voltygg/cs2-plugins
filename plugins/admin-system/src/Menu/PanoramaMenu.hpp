#pragma once

#include "MenuIds.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/PerSlot.hpp>
#include <VoltMod/Core/Subscription.hpp>
#include <VoltMod/Core/SubscriptionScope.hpp>
#include <VoltMod/Entities/MovementFreeze.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Menu/MenuStack.hpp>
#include <VoltMod/Menu/MenuState.hpp>
#include <VoltMod/Runtime.hpp>
#include <VoltMod/Ui/Screen.hpp>
#include <VoltMod/Ui/UiClick.hpp>
#include <VoltMod/Ui/UiPanel.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace AdminSystem::Menus
{

/**
 * @brief The admin menu drawn on this plugin's own Panorama screen, clicked rather than typed.
 *
 * A @ref VoltMod::MenuSession like the framework's, so the same MenuBuilder rows, ActionRows and
 * Flow steps run against it unchanged. The stack, the breadcrumb, how a row describes itself and
 * how a stepped value is held back are the framework's @ref VoltMod::MenuStack, shared with the
 * center-HTML menu. What is this plugin's own is the shape: a tab strip over the root menu's
 * submenus, a fixed page of rows, and the click ids that address them.
 *
 * Each player gets a private panel, so a spectator sees their own menu rather than the one
 * belonging to the pawn they are watching. A player who cannot be drawn to is refused by
 * @ref Begin, which is the caller's cue to fall back to the framework's center-HTML menu.
 */
class PanoramaMenu final : public VoltMod::MenuSession
{
public:
    explicit PanoramaMenu(VoltMod::Runtime& runtime);
    ~PanoramaMenu() override;

    /** Arm click handling when @p enabled, and require @p addonId of connecting clients so they
     *  have the layout. A zero id requires nothing, for a client compiled into by hand. */
    void Start(bool enabled, uint64_t addonId);

    /** Whether a session opened now for @p slot would be drawn here.
     *
     *  Spawns the player's panel to find out, so a caller that builds rows against this host
     *  before opening gets the same answer the open will. Cheap after the first call. */
    [[nodiscard]] bool Available(int slot);

    /** Start a session for @p slot showing @p menu, replacing any it has open.
     *  False when the panel could not be shown, so the caller can use center HTML instead. */
    bool Begin(int slot, std::shared_ptr<VoltMod::Menu> menu, VoltMod::MenuOptions options = {});

    [[nodiscard]] bool IsOpen(int slot) const;

    void Open(int slot, std::shared_ptr<VoltMod::Menu> menu) override;
    void Close(int slot) override;
    void CloseAll(int slot) override;
    void CloseAll(int slot, std::string_view replyKey) override;
    void Prompt(int slot, std::string prompt, std::function<bool(int slot, std::string_view text)> callback) override;
    [[nodiscard]] std::string Translate(int slot, std::string_view key, std::string_view fallback) const override;

private:
    /** One tab of the strip: which root item it opens, and the label that item described itself
     *  with when the session started. */
    struct Tab
    {
        int Item;
        std::string Label;
    };

    /** What this surface adds on top of the shared stack: where the player is on screen. */
    struct Session
    {
        /** The root menu's submenu rows, in tab order. */
        std::vector<Tab> Tabs;
        /** Which tab the open branch was entered through, or -1. */
        int OpenTab = -1;
        int Page = 0;
        /** The pawn this session is holding still, if any. */
        VoltMod::MovementFreeze Freeze;
    };

    [[nodiscard]] int ItemIndex(int slot, int row) const;

    void Draw(int slot);
    void DrawHeader(int slot, const VoltMod::Menu& menu);
    void DrawRows(int slot, const VoltMod::Menu& menu);
    void DrawRow(int slot, int row, const VoltMod::MenuRow& described);
    void DrawTabs(int slot);
    void DrawPrompt(int slot);

    void OnClick(const VoltMod::UiClick& click);

    /** Run item @p index of the open menu, remembering which tab it belongs to. */
    void Activate(int slot, int index);
    void Nudge(int slot, int row, int direction);
    void OpenTab(int slot, int tab);
    void TurnPage(int slot, int delta);

    /** Take the menu off @p slot's screen, cancel its prompt, and let its pawn go. */
    void Dismiss(int slot);

    VoltMod::Runtime& _rt;
    /** False until Start turns the surface on. */
    bool _enabled = false;
    /** The addon requirement, held for as long as this plugin is loaded. */
    VoltMod::Subscription _addon;
    /** The half every menu surface shares: stack, breadcrumb, Describe, Activate and Step. */
    VoltMod::MenuStack _stack;
    /** One panel per player: a spectating admin must see their own menu, not the pawn's. */
    VoltMod::Screen _screen{_rt.Ui, _rt.Slots, AdminUi::Menu::Layout, AdminUi::Menu::RootId};
    VoltMod::PerSlot<Session> _sessions;
    /** Declared last: click delivery drops before the state it touches. */
    VoltMod::SubscriptionScope _subs;
};

}  // namespace AdminSystem::Menus
