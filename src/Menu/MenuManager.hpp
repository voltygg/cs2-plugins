#pragma once

#include "Menu.hpp"
#include "../Core/Singleton.hpp"
#include <array>

namespace AdminSystem::Menu {

class MenuManager : public Core::Singleton<MenuManager> {
    friend class Core::Singleton<MenuManager>;

public:
    void OpenMenu(int slot, std::shared_ptr<Menu> menu);
    void CloseMenu(int slot);
    void CloseAllMenus(int slot);
    bool HasActiveMenu(int slot) const;
    void OnGameFrame();
    void OnPlayerDisconnect(int slot);

private:
    MenuManager() = default;

    void HandleInput(int slot, uint64_t buttons, uint64_t prevButtons);
    void RenderMenu(int slot);

    std::array<PlayerMenuState, 64> _states;
    static constexpr int64_t InputDebounceMs = 200;
};

} // namespace AdminSystem::Menu
