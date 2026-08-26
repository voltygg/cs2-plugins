#pragma once

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <functional>
#include <memory>
#include <string>

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<VoltMod::MenuView> BuildPunishMenu(AdminSystem::App& app, int adminSlot);
std::shared_ptr<VoltMod::MenuView> BuildPunishActionsMenu(AdminSystem::App& app, int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Menu
