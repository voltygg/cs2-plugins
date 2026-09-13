#pragma once

#include "MenuWriters.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/PerSlot.hpp>
#include <VoltMod/Core/Subscription.hpp>
#include <VoltMod/Core/Subscriptions.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Menu/MenuStack.hpp>
#include <VoltMod/Runtime.hpp>
#include <VoltMod/Ui/Screen.hpp>
#include <VoltMod/Ui/UiClick.hpp>
#include <VoltMod/Ui/UiPanel.hpp>
#include <VoltMod/Ui/Writers.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace AdminSystem::Menus
{

/**
 * @brief The admin menu on this plugin's Panorama screen, clicked rather than typed.
 *
 * A @ref VoltMod::MenuSurface over the shared @ref VoltMod::MenuStack, so the same rows run on it
 * and on center HTML. Each player gets a private panel, so a spectator sees their own menu.
 */
class PanoramaMenu final : public VoltMod::MenuSurface
{
public:
    explicit PanoramaMenu(VoltMod::Runtime& runtime);
    ~PanoramaMenu() override;

    /** Turn click handling on when @p enabled, and require @p addonId of connecting clients so they
     *  have the layout. A zero id requires nothing, for a client compiled into by hand. */
    void Start(bool enabled, uint64_t addonId);

    /** Whether a session opened now for @p slot would be drawn here. Spawns the panel to find out. */
    [[nodiscard]] bool CanDraw(int slot);

    /** Start a session for @p slot showing @p menu. False means fall back to center HTML. */
    bool Open(int slot, std::shared_ptr<VoltMod::Menu> menu, VoltMod::MenuOptions options);

    [[nodiscard]] bool IsOpen(int slot) const;

    void Open(int slot, std::shared_ptr<VoltMod::Menu> menu) override;
    void Close(int slot) override;
    void CloseAll(int slot) override;
    void CloseAll(int slot, std::string_view replyKey) override;
    void Prompt(int slot, std::string prompt, std::function<bool(int slot, std::string_view text)> callback) override;
    [[nodiscard]] std::string Translate(int slot, std::string_view key, std::string_view fallback) const override;

private:
    /** A sidebar tab and the root row it opens, read once when the session starts. */
    struct Tab
    {
        int RootIndex;
        std::string Label;
        /** An index into AdminUi::Menu::IconClasses, or ClassChoice::None. */
        int Icon;
    };

    struct Session
    {
        std::vector<Tab> Tabs;
        /** The tab the open branch was entered through, or -1. */
        int SelectedTab = -1;
        int Page = 0;
    };

    [[nodiscard]] int ItemAt(int slot, int row) const;

    void Draw(int slot);
    void DrawHeader(const VoltMod::UiPanelWriter& w, const VoltMod::Menu& menu);
    void DrawTabs(const VoltMod::UiPanelWriter& w);
    void DrawRows(const VoltMod::UiPanelWriter& w, const VoltMod::Menu& menu);
    void DrawRow(const VoltMod::UiPanelWriter& w, int row, const VoltMod::MenuRow& described);
    void DrawPrompt(const VoltMod::UiPanelWriter& w);

    void OnClick(const VoltMod::UiClick& click);

    /** Run item @p index of the open menu, remembering which tab it belongs to. */
    void Activate(int slot, int index);
    void StepRow(int slot, int row, int direction);
    void OpenTab(int slot, int tab);
    void TurnPage(int slot, int delta);

    /** Take the menu off @p slot's screen, cancel its prompt, and let its pawn go. */
    void Hide(int slot);

    VoltMod::Runtime& _rt;
    bool _enabled = false;
    /** The addon requirement, held while the plugin is loaded. */
    VoltMod::Subscription _addon;
    VoltMod::MenuStack _stack;
    VoltMod::Screen _screen{_rt.Ui, _rt.Slots, AdminUi::Menu::Layout, AdminUi::Menu::RootId};
    VoltMod::PerSlot<Session> _sessions;
    /** Declared last: click delivery drops before the state it touches. */
    VoltMod::Subscriptions _subs;
};

}  // namespace AdminSystem::Menus
