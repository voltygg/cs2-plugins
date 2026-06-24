#pragma once

#include <CS2Kit/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<::CS2Kit::Menu::Menu> BuildTeamPickerMenu(int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Menu
