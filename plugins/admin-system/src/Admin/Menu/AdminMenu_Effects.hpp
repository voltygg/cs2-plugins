#pragma once

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<VoltMod::Menu> BuildEffectsMenu(AdminSystem::App& app, int adminSlot);
std::shared_ptr<VoltMod::Menu> BuildEffectsActionsMenu(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                                       VoltMod::PlayerRef target);

}  // namespace AdminSystem::Admin::Menu
