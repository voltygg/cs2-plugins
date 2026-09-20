#pragma once

#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Player Fun: one row per connected player. */
std::shared_ptr<VoltMod::Menu> BuildPlayerFunTab(AdminSystem::App& app, int adminSlot);

/** The cosmetic and joke effects one admin may apply to one player. */
std::shared_ptr<VoltMod::Menu> BuildPlayerFunCard(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                                  VoltMod::PlayerRef target);

}  // namespace AdminSystem::Admin::Menu
