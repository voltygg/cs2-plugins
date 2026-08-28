#pragma once

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Paginated list of active bans; selecting one opens an unban confirmation. */
std::shared_ptr<VoltMod::MenuView> BuildUnbanMenu(AdminSystem::App& app, int adminSlot);

/** Paginated list of active voice and text mutes; selecting one opens an unmute confirmation. */
std::shared_ptr<VoltMod::MenuView> BuildUnmuteMenu(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin::Menu
