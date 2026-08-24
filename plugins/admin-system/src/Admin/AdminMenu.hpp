#pragma once

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Admin
{

/** Build the top-level admin panel menu (Punish / Control / Effects / Mini-games / Round). */
std::shared_ptr<VoltMod::MenuView> BuildAdminMainMenu(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin
