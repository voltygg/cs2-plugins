#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/MenuModel.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Weapon picker for one target: every configured weapon, a random pick, and strip. */
std::shared_ptr<VoltMod::Menu> BuildWeaponPicker(const MenuContext& ctx, VoltMod::PlayerRef target);

}  // namespace AdminSystem::Admin::Menu
