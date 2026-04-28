#pragma once

#include <CS2Kit/Menu/Menu.hpp>
#include <functional>
#include <memory>
#include <string>

namespace AdminSystem::Admin::Menu
{

using namespace CS2Kit::Menu;

std::shared_ptr<::CS2Kit::Menu::Menu> BuildPunishMenu(int adminSlot);
std::shared_ptr<::CS2Kit::Menu::Menu> BuildPunishActionsMenu(int adminSlot, int targetSlot);
std::shared_ptr<::CS2Kit::Menu::Menu> BuildDurationMenu(int adminSlot, int targetSlot, const std::string& actionName,
                                                        std::function<void(int, int, int)> onDuration);

}  // namespace AdminSystem::Admin::Menu
