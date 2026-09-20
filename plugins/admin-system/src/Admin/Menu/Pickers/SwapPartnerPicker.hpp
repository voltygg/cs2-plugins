#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Second picker of a swap: who @p first trades places with. */
std::shared_ptr<VoltMod::Menu> BuildSwapPartnerPicker(const MenuContext& ctx, VoltMod::PlayerRef first);

}  // namespace AdminSystem::Admin::Menu
