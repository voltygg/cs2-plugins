#pragma once

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<VoltMod::MenuView> BuildTeamPickerMenu(AdminSystem::App& app, int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Menu
