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

std::shared_ptr<VoltMod::MenuView> BuildEffectsMenu(AdminSystem::App& app, int adminSlot);
std::shared_ptr<VoltMod::MenuView> BuildEffectsActionsMenu(AdminSystem::App& app, int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Menu
