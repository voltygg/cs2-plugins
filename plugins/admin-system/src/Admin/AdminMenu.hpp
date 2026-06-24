#pragma once

#include <CS2Kit/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin
{

using namespace CS2Kit::Menu;

/** Build the top-level admin panel menu (Punish / Control / Effects / Mini-games / Round). */
std::shared_ptr<::CS2Kit::Menu::Menu> BuildAdminMainMenu(int adminSlot);

}  // namespace AdminSystem::Admin
