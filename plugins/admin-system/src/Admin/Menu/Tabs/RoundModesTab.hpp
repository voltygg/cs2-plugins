#pragma once

#include "Core/App.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Round Modes: one toggle row per server-wide round modifier, plus a clear-all row. */
std::shared_ptr<VoltMod::Menu> BuildRoundModesTab(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin::Menu
