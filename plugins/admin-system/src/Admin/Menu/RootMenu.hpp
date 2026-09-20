#pragma once

#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Build the top-level admin panel: one submenu row per tab the viewer may open. */
std::shared_ptr<VoltMod::Menu> BuildRootMenu(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin::Menu
