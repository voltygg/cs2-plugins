#pragma once

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<VoltMod::MenuView> BuildControlMenu(AdminSystem::App& app, int adminSlot);
std::shared_ptr<VoltMod::MenuView> BuildControlActionsMenu(AdminSystem::App& app, int adminSlot, int targetSlot);
/** Weapon picker for one target: every configured weapon, a random pick, and strip. */
std::shared_ptr<VoltMod::MenuView> BuildWeaponMenu(AdminSystem::App& app, int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Menu
