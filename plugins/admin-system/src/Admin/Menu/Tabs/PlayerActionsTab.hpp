#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/MenuModel.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Player Actions: one row per connected player. */
std::shared_ptr<VoltMod::Menu> BuildPlayerActionsTab(const MenuContext& ctx);

/** Everything one admin may do to one player: cheat check, vitals, movement, team, weapons. */
std::shared_ptr<VoltMod::Menu> BuildPlayerActionsCard(const MenuContext& ctx, VoltMod::PlayerRef target);

}  // namespace AdminSystem::Admin::Menu
