#pragma once

#include <Ui/AdminMenu.hpp>
#include <VoltMod/Core/PerSlot.hpp>
#include <VoltMod/Core/SlotEvents.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Ui/Screen.hpp>
#include <VoltMod/Ui/ScreenManager.hpp>
#include <optional>
#include <string_view>

namespace AdminSystem::Menus
{

/** What a pressed button on the admin menu layout stands for. */
enum class MenuButtonKind
{
    Cancel,
    Back,
    Close,
    PreviousPage,
    NextPage,
    Tab,
    Row,
    StepDown,
    StepUp,
};

struct MenuButton
{
    MenuButtonKind Kind;
    /** The tab or row index, for the kinds that have one. */
    int Index = 0;
};

/** The text above the rows. An empty subtitle hides its line. */
struct MenuHeader
{
    std::string_view Brand;
    std::string_view BrandSubtitle;
    std::string_view Breadcrumb;
    std::string_view Title;
    std::string_view Subtitle;
};

/**
 * @brief The admin menu layout, drawn for one player at a time on a screen only they receive.
 *
 * Knows the layout's element ids and classes and nothing about menus. A player screen keeps working
 * while its owner is dead or spectating.
 */
class AdminMenuScreen
{
public:
    static constexpr int RowCount = static_cast<int>(AdminMenuLayout::Rows.size());
    static constexpr int TabCount = static_cast<int>(AdminMenuLayout::Tabs.size());

    AdminMenuScreen(VoltMod::ScreenManager& screens, VoltMod::SlotEvents& slots);

    /** Spawn the screen if needed, unhide it and give the cursor. False when @p slot cannot be drawn to. */
    bool Show(int slot);
    void Hide(int slot);

    void SetHeader(int slot, const MenuHeader& header);
    void SetSidebarVisible(int slot, bool visible);
    void SetTab(int slot, int index, std::string_view label, std::string_view icon, bool selected);
    void HideTab(int slot, int index);
    void SetRow(int slot, int index, const VoltMod::MenuRow& row, std::string_view pendingHint);
    void HideRow(int slot, int index);
    void ShowEmpty(int slot, std::string_view text);
    void HideEmpty(int slot);
    void ShowPager(int slot, std::string_view text);
    void HidePager(int slot);
    void ShowPrompt(int slot, std::string_view text, std::string_view hint);
    void HidePrompt(int slot);
    void SetFooter(int slot, std::string_view back, std::string_view cancel);

    /** The button a pressed id names, or nothing for an id outside this layout. */
    [[nodiscard]] static std::optional<MenuButton> ButtonFor(std::string_view id);

private:
    /** @p slot's screen, created on first use; an empty one when it cannot be. */
    VoltMod::Screen& ScreenFor(int slot);

    VoltMod::ScreenManager& _manager;
    VoltMod::PerSlot<std::optional<VoltMod::Screen>> _screens;
    /** Stands in when a player screen cannot be created; every write to it fails quietly. */
    VoltMod::Screen _empty;
};

}  // namespace AdminSystem::Menus
