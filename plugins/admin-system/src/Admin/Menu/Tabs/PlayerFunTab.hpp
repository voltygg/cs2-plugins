#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/MenuModel.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Player Fun: one row per connected player. */
std::shared_ptr<VoltMod::Menu> BuildPlayerFunTab(const MenuContext& ctx);

/** The cosmetic and joke effects one admin may apply to one player. */
std::shared_ptr<VoltMod::Menu> BuildPlayerFunCard(const MenuContext& ctx, VoltMod::PlayerRef target);

}  // namespace AdminSystem::Admin::Menu
