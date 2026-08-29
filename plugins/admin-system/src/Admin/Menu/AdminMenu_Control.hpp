#pragma once

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<VoltMod::Menu> BuildControlMenu(AdminSystem::App& app, int adminSlot);
std::shared_ptr<VoltMod::Menu> BuildControlActionsMenu(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                                       VoltMod::PlayerRef target);
/** Weapon picker for one target: every configured weapon, a random pick, and strip. */
std::shared_ptr<VoltMod::Menu> BuildWeaponMenu(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                               VoltMod::PlayerRef target);

}  // namespace AdminSystem::Admin::Menu
