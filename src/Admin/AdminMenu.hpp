#pragma once

#include "../../vendor/cs2-kit/src/Menu/Menu.hpp"

#include <memory>

namespace AdminSystem::Admin
{

/** Build the top-level admin panel menu (Player Management, Server Management). */
std::shared_ptr<CS2Kit::Menu::Menu> BuildAdminMainMenu(int adminSlot);
/** Build a menu listing all connected players for the admin to act on. */
std::shared_ptr<CS2Kit::Menu::Menu> BuildPlayerListMenu(int adminSlot);
/** Build the per-player actions menu (Kick, Ban, Mute, Gag, Warn). */
std::shared_ptr<CS2Kit::Menu::Menu> BuildPlayerActionsMenu(int adminSlot, int targetSlot);
/** Build a duration selection submenu for timed punishments. */
std::shared_ptr<CS2Kit::Menu::Menu> BuildDurationMenu(int adminSlot, int targetSlot, const std::string& actionName,
                                              std::function<void(int, int, int)> onDuration);
/** Build the server management submenu (changemap, restart round, etc.). */
std::shared_ptr<CS2Kit::Menu::Menu> BuildServerMenu(int adminSlot);

}  // namespace AdminSystem::Admin
