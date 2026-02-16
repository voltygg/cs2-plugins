#pragma once

#include "../Menu/Menu.hpp"
#include <memory>

namespace AdminSystem::Admin {

std::shared_ptr<Menu::Menu> BuildAdminMainMenu(int adminSlot);
std::shared_ptr<Menu::Menu> BuildPlayerListMenu(int adminSlot);
std::shared_ptr<Menu::Menu> BuildPlayerActionsMenu(int adminSlot, int targetSlot);
std::shared_ptr<Menu::Menu> BuildDurationMenu(int adminSlot, int targetSlot,
                                                const std::string& actionName,
                                                std::function<void(int, int, int)> onDuration);
std::shared_ptr<Menu::Menu> BuildServerMenu(int adminSlot);

} // namespace AdminSystem::Admin
