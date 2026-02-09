#pragma once

#include "menu.h"
#include <memory>

namespace menus {

/**
 * @brief Build the admin main menu for a given admin slot.
 * Items are filtered based on the admin's permissions.
 */
std::shared_ptr<Menu> BuildAdminMainMenu(int admin_slot);

/**
 * @brief Build the player list submenu.
 * Lists all connected players that the admin can target.
 */
std::shared_ptr<Menu> BuildPlayerListMenu(int admin_slot);

/**
 * @brief Build the player actions submenu for a specific target.
 * Shows Kick, Ban, Mute, Gag, Warn options.
 */
std::shared_ptr<Menu> BuildPlayerActionsMenu(int admin_slot, int target_slot);

/**
 * @brief Build the duration selection submenu.
 * Used before executing Ban, Mute, or Gag.
 * @param action_name Display name of the action (e.g. "Ban")
 * @param on_duration Called with the selected duration in seconds (0 = permanent)
 */
std::shared_ptr<Menu> BuildDurationMenu(int admin_slot, int target_slot,
                                         const std::string& action_name,
                                         std::function<void(int slot, int target, int duration)> on_duration);

/**
 * @brief Build the server management submenu.
 */
std::shared_ptr<Menu> BuildServerMenu(int admin_slot);

} // namespace menus
