#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin
{

/** Build the top-level admin panel menu (Punish / Control / Effects / Mini-games / Round). */
std::shared_ptr<CS2Kit::MenuView> BuildAdminMainMenu(int adminSlot);

}  // namespace AdminSystem::Admin
