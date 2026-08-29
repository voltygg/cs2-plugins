#pragma once

#include "../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin
{

/** Build the top-level admin panel menu (Punish / Control / Effects / Mini-games / Round). */
std::shared_ptr<VoltMod::Menu> BuildAdminMainMenu(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin
