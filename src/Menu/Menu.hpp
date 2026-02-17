#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace AdminSystem::Menu
{

/**
 * Optional custom header/footer renderers for a menu.
 * If null, the default Gold & Ember theme is used.
 */
struct MenuLayout
{
    std::function<std::string()> Header;
    std::function<std::string()> Footer;
};

/** A single selectable item in a menu. */
struct MenuItem
{
    std::string Title;
    std::function<void(int)> OnSelect;
    bool Enabled = true;
};

/** A complete menu definition with title, items, close callback, and layout overrides. */
struct Menu
{
    std::string Title;
    std::vector<MenuItem> Items;
    std::function<void(int)> OnClose;
    MenuLayout Layout;
};

/** Per-player menu state: tracks the menu stack, current selection, and input timing. */
struct PlayerMenuState
{
    std::stack<std::shared_ptr<Menu>> MenuStack;
    int SelectedIndex = 0;
    int64_t LastInputTime = 0;
    uint64_t PrevButtons = 0;

    bool HasMenu() const { return !MenuStack.empty(); }
    Menu* GetCurrentMenu() { return MenuStack.empty() ? nullptr : MenuStack.top().get(); }

    void Reset()
    {
        while (!MenuStack.empty())
            MenuStack.pop();
        SelectedIndex = 0;
        LastInputTime = 0;
        PrevButtons = 0;
    }
};

}  // namespace AdminSystem::Menu
