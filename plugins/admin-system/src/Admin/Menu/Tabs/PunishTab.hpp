#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Punish: the active bans and mutes, then one row per connected player. */
std::shared_ptr<VoltMod::Menu> BuildPunishTab(const MenuContext& ctx);

/** The punishments one admin may issue against one player. */
std::shared_ptr<VoltMod::Menu> BuildPunishCard(const MenuContext& ctx, VoltMod::PlayerRef target);

}  // namespace AdminSystem::Admin::Menu
