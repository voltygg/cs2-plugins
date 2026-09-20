#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** My Settings: this admin's own hide toggle, chat prefix and colors. */
std::shared_ptr<VoltMod::Menu> BuildMySettingsTab(const MenuContext& ctx);

}  // namespace AdminSystem::Admin::Menu
