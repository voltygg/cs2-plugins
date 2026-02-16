#pragma once

#include <functional>
#include <string>
#include <vector>
#include <stack>
#include <cstdint>
#include <memory>

namespace menus {

/**
 * @brief A single item in a menu.
 */
struct MenuItem
{
    std::string title;
    std::function<void(int slot)> on_select;
    bool enabled{true};
};

/**
 * @brief A menu definition with title and items.
 */
struct Menu
{
    std::string title;
    std::vector<MenuItem> items;
    std::function<void(int slot)> on_close;
};

/**
 * @brief Per-player menu state tracking.
 */
struct PlayerMenuState
{
    std::stack<std::shared_ptr<Menu>> menu_stack;
    int selected_index{0};
    int64_t last_input_time{0};
    uint64_t prev_buttons{0};

    bool HasMenu() const { return !menu_stack.empty(); }
    Menu* GetCurrentMenu() { return menu_stack.empty() ? nullptr : menu_stack.top().get(); }

    void Reset()
    {
        while (!menu_stack.empty()) menu_stack.pop();
        selected_index = 0;
        last_input_time = 0;
        prev_buttons = 0;
    }
};

} // namespace menus
