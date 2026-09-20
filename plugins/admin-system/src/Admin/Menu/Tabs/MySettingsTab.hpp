#pragma once

#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** My Settings: this admin's own chat prefix and colors. */
std::shared_ptr<VoltMod::Menu> BuildMySettingsTab(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin::Menu
