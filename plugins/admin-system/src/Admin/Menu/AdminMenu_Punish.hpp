#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Menu/Menu.hpp>
#include <functional>
#include <memory>
#include <string>

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<CS2Kit::MenuView> BuildPunishMenu(int adminSlot);
std::shared_ptr<CS2Kit::MenuView> BuildPunishActionsMenu(int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Menu
