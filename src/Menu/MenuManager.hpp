#pragma once

#include "Menu.hpp"
#include "../Core/Singleton.hpp"
#include <array>

namespace AdminSystem::Menu {

/**
 * Manages WASD-navigated center-HTML menus for all players.
 * Supports a per-player menu stack (submenus push, R pops back).
 * Driven by OnGameFrame() — reads button state each tick for input.
 */
class MenuManager : public Core::Singleton<MenuManager> {
public:
    explicit MenuManager(Token) {}

    void OpenMenu(int slot, std::shared_ptr<Menu> menu);
    void CloseMenu(int slot);
    void CloseAllMenus(int slot);
    bool HasActiveMenu(int slot) const;
    void OnGameFrame();
    void OnPlayerDisconnect(int slot);

private:
    void HandleInput(int slot, uint64_t buttons, uint64_t prevButtons);
    void RenderMenu(int slot);

    std::array<PlayerMenuState, 64> _states;
    static constexpr int64_t InputDebounceMs = 200;
};

} // namespace AdminSystem::Menu
