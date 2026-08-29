#pragma once

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/**
 * @brief Build a paginated picker listing every connected player for @p adminSlot.
 *
 * The framework picker with this plugin's "nobody connected" label filled in; @p spec supplies
 * the title, what a pick does, and - optionally - which rows render disabled (e.g. to gray out
 * an already-picked player). The viewer is @p adminSlot, so `Pick` receives only the target.
 */
std::shared_ptr<VoltMod::Menu> BuildPlayerPicker(AdminSystem::App& app, int adminSlot, VoltMod::PlayerPicker spec);

}  // namespace AdminSystem::Admin::Menu
