#pragma once

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Paginated list of active bans; selecting one opens an unban confirmation dialog. */
std::shared_ptr<VoltMod::MenuView> BuildUnbanMenu(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin::Menu
