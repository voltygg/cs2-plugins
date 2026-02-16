#pragma once

#include "menu.h"
#include <array>
#include <mutex>

namespace menus {

/**
 * @brief Menu manager singleton. Handles menu lifecycle, WASD input, and rendering.
 *
 * Call OnGameFrame() every server frame to poll player buttons and update menus.
 */
class MenuManager
{
public:
    /**
     * @brief Open a menu for a player. Pushes onto the menu stack.
     * @param slot Player slot (0-63)
     * @param menu Shared pointer to the menu to display
     */
    void OpenMenu(int slot, std::shared_ptr<Menu> menu);

    /**
     * @brief Close the current menu for a player (pop one level).
     * If the stack is empty after popping, the menu is fully closed.
     * @param slot Player slot (0-63)
     */
    void CloseMenu(int slot);

    /**
     * @brief Close all menus for a player (clear the entire stack).
     * @param slot Player slot (0-63)
     */
    void CloseAllMenus(int slot);

    /**
     * @brief Check if a player has an active menu.
     */
    bool HasActiveMenu(int slot) const;

    /**
     * @brief Called every server frame. Polls buttons and handles menu input.
     */
    void OnGameFrame();

    /**
     * @brief Called when a player disconnects. Cleans up menu state.
     */
    void OnPlayerDisconnect(int slot);

    /**
     * @brief Get singleton instance.
     */
    static MenuManager& Instance();

private:
    MenuManager() = default;
    ~MenuManager() = default;
    MenuManager(const MenuManager&) = delete;
    MenuManager& operator=(const MenuManager&) = delete;

    void HandleInput(int slot, uint64_t buttons, uint64_t prev_buttons);
    void RenderMenu(int slot);

    std::array<PlayerMenuState, 64> m_states;
    mutable std::recursive_mutex m_mutex;

    // Input debounce interval in milliseconds
    static constexpr int64_t INPUT_DEBOUNCE_MS = 200;
};

} // namespace menus
