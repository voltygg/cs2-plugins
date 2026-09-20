#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Round Modes: one toggle row per server-wide round modifier, plus a clear-all row. */
std::shared_ptr<VoltMod::Menu> BuildRoundModesTab(const MenuContext& ctx);

}  // namespace AdminSystem::Admin::Menu
