#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Paginated list of active bans; selecting one opens an unban confirmation dialog. */
std::shared_ptr<CS2Kit::MenuView> BuildUnbanMenu(int adminSlot);

}  // namespace AdminSystem::Admin::Menu
