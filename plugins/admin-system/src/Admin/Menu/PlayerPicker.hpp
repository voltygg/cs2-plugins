#pragma once

#include <CS2Kit/Menu/Menu.hpp>
#include <functional>
#include <memory>
#include <string>

namespace AdminSystem::Admin::Menu
{

/**
 * @brief Build a paginated picker listing every connected player.
 * Selecting a player invokes onPick(adminSlot, targetSlot) which is expected
 * to open the appropriate per-target actions submenu.
 */
std::shared_ptr<::CS2Kit::Menu::Menu> BuildPlayerPicker(int adminSlot, const std::string& title,
                                                        std::function<void(int adminSlot, int targetSlot)> onPick);

}  // namespace AdminSystem::Admin::Menu
