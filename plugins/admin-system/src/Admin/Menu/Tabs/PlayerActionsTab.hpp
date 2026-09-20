#pragma once

#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Player Actions: the self Hide toggle, then one row per connected player. */
std::shared_ptr<VoltMod::Menu> BuildPlayerActionsTab(AdminSystem::App& app, int adminSlot);

/** Everything one admin may do to one player: cheat check, vitals, movement, team, weapons. */
std::shared_ptr<VoltMod::Menu> BuildPlayerActionsCard(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                                      VoltMod::PlayerRef target);

}  // namespace AdminSystem::Admin::Menu
