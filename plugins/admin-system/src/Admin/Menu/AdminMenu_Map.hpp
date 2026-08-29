#pragma once

#include "../../Core/App.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Map picker: every configured map, each confirming before it takes the server away. */
std::shared_ptr<VoltMod::Menu> BuildMapMenu(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin::Menu
