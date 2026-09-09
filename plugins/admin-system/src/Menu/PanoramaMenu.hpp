#pragma once

#include "MenuIds.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/PerSlot.hpp>
#include <VoltMod/Core/Subscription.hpp>
#include <VoltMod/Core/SubscriptionScope.hpp>
#include <VoltMod/Entities/EntityRef.hpp>
#include <VoltMod/Engine/EngineTypes.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Runtime.hpp>
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
 * Flow steps run against it unchanged. It keeps its own stack because it needs neither cursor nor
 * keys: every press names the row it came from.
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
    bool Begin(int slot, std::shared_ptr<VoltMod::Menu> menu);

    [[nodiscard]] bool IsOpen(int slot) const;

    void Open(int slot, std::shared_ptr<VoltMod::Menu> menu) override;
    void Close(int slot) override;
    void CloseAll(int slot) override;
    void CloseAll(int slot, std::string_view replyKey) override;
    void Prompt(int slot, std::string prompt, std::function<bool(int slot, std::string_view text)> callback) override;
    [[nodiscard]] std::string Translate(int slot, std::string_view key, std::string_view fallback) const override;

private:
    /** How long a stepped value waits before it is applied, so a burst of presses is one action. */
    static constexpr int CommitDelayMs = 400;

    /** One player's open menus and where they are in them. */
    struct Session
    {
        std::vector<std::shared_ptr<VoltMod::Menu>> Stack;
        /** Item indexes of the root menu's submenu rows, in tab order. */
        std::vector<int> Tabs;
        /** Which tab the open branch was entered through, or -1. */
        int OpenTab = -1;
        int Page = 0;
        /** Item index of the row holding a stepped value that has not been applied yet, or -1. */
        int PendingItem = -1;
        VoltMod::Subscription CommitTimer;
        /** The pawn held frozen, or unset. Only this pawn is given @ref PrevMove back. */
        VoltMod::EntityRef FrozenPawn;
        VoltMod::MoveType PrevMove = VoltMod::MoveType::Walk;
    };

    /** @p slot's own panel, made on first use. Empty when it could not be spawned. */
    VoltMod::UiPanel& PanelFor(int slot);

    [[nodiscard]] VoltMod::Menu* Current(int slot);
    [[nodiscard]] int ItemIndex(int slot, int row) const;

    void Draw(int slot);
    void DrawHeader(int slot, const VoltMod::Menu& menu);
    void DrawRows(int slot, VoltMod::Menu& menu);
    void DrawRow(int slot, int row, int item, const VoltMod::MenuRow& described);
    void DrawTabs(int slot);
    void DrawPrompt(int slot);

    void OnClick(const VoltMod::UiClick& click);
    void Press(int slot, int row);

    /** Run item @p index of the open menu, wherever it sits on the page. */
    void Activate(int slot, int index);
    void Nudge(int slot, int row, int direction);
    void OpenTab(int slot, int tab);
    void TurnPage(int slot, int delta);

    /** Apply whatever a stepped row was left showing, and forget it. */
    void RunPending(int slot);

    /** Take the menu off @p slot's screen, cancel its prompt, and let its pawn go. */
    void Dismiss(int slot);
    void Freeze(int slot, bool on);

    /** The titles under the top menu, joined; empty at the root. */
    [[nodiscard]] std::string Breadcrumb(int slot) const;

    VoltMod::Runtime& _rt;
    /** False until Start turns the surface on. */
    bool _enabled = false;
    /** The addon requirement, held for as long as this plugin is loaded. */
    VoltMod::Subscription _addon;
    VoltMod::PerSlot<VoltMod::UiPanel> _panels;
    VoltMod::PerSlot<Session> _sessions;
    /** Declared last: click delivery drops before the state it touches. */
    VoltMod::SubscriptionScope _subs;
};

}  // namespace AdminSystem::Menus
