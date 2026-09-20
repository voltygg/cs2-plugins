#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Team picker for one target: CT, T or spectator. */
std::shared_ptr<VoltMod::Menu> BuildTeamPicker(const MenuContext& ctx, VoltMod::PlayerRef target);

}  // namespace AdminSystem::Admin::Menu
