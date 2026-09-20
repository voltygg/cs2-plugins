#pragma once

#include "Admin/Menu/MenuContext.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <functional>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/**
 * @brief Build a paginated picker listing every connected player for @p adminSlot.
 *
 * The framework picker with this plugin's "nobody connected" label filled in; @p spec supplies
 * the title, what a pick does, and - optionally - which rows render disabled (e.g. to gray out
 * an already-picked player). The viewer is @p adminSlot, so a pick receives only the target.
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

/** @ref AppendPlayerRows for a list whose rows act on the player, each opening @p open. A player
 *  this admin may not touch is grayed here, once, rather than on every row of the card. */
void AppendTargetRows(const MenuContext& ctx, VoltMod::MenuBuilder& builder,
                      std::function<std::shared_ptr<VoltMod::Menu>(VoltMod::PlayerRef target)> open);

}  // namespace AdminSystem::Admin::Menu
