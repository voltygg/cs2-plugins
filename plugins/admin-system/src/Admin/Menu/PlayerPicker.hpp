#pragma once

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
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

/**
 * @brief @ref BuildPlayerPicker's rows, appended to a builder that already has rows of its own.
 *
 * Wraps the framework's `AppendPlayerRows` the same way, so both entry points fill the plugin's
 * defaults in one place: a caller that appends the list into its own menu gets the same
 * "nobody connected" label as one that opens a picker.
 */
void AppendPlayerRows(AdminSystem::App& app, int adminSlot, VoltMod::MenuBuilder& builder, VoltMod::PlayerPicker spec);

}  // namespace AdminSystem::Admin::Menu
