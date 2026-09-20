#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Map & Vote: change the map, queue the next one, put one to the players, or cancel a vote. */
std::shared_ptr<VoltMod::Menu> BuildMapVoteTab(const MenuContext& ctx);

}  // namespace AdminSystem::Admin::Menu
