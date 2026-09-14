#pragma once

#include <Ui/AdminMenu.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Menu/MenuLayout.hpp>
#include <VoltMod/Ui/PlayerScreens.hpp>
#include <VoltMod/Ui/ScreenManager.hpp>
#include <optional>
#include <string_view>

namespace AdminSystem::Menus
{

/**
 * @brief The admin menu layout, drawn for one player at a time on a screen only they receive.
 *
 * Knows the layout's element ids and classes and nothing about menus. A player screen keeps working
 * while its owner is dead or spectating.
 */
class AdminMenuScreen final : public VoltMod::MenuLayout
{
public:
    explicit AdminMenuScreen(VoltMod::ScreenManager& screens);

    [[nodiscard]] int RowCount() const override { return static_cast<int>(AdminMenuLayout::Rows.size()); }
    [[nodiscard]] int TabCount() const override { return static_cast<int>(AdminMenuLayout::Tabs.size()); }

    bool Show(int slot) override;
    void Hide(int slot) override;

    void SetHeader(int slot, const VoltMod::MenuHeader& header) override;
    void SetSidebarVisible(int slot, bool visible) override;
    void SetTab(int slot, int index, const VoltMod::MenuTab* tab) override;
    void SetRow(int slot, int index, const VoltMod::MenuRow* row, std::string_view pendingHint) override;
    void SetEmpty(int slot, std::string_view text) override;
    void SetPager(int slot, std::string_view text) override;
    void SetPrompt(int slot, std::string_view text, std::string_view hint) override;
    void SetFooter(int slot, std::string_view back, std::string_view cancel) override;

    [[nodiscard]] std::optional<VoltMod::MenuButton> ButtonFor(std::string_view id) const override;

private:
    VoltMod::PlayerScreens _screens;
};

}  // namespace AdminSystem::Menus
