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

/** The framework's player picker with this plugin's "nobody connected" label filled in. */
std::shared_ptr<VoltMod::Menu> BuildPlayerPicker(AdminSystem::App& app, int adminSlot, VoltMod::PlayerPicker spec);

/** A list whose rows act on the player, each opening @p open. A player this admin may not touch
 *  is grayed here, once, rather than on every row of the card behind it. */
void AppendTargetRows(const MenuContext& ctx, VoltMod::MenuBuilder& builder,
                      std::function<std::shared_ptr<VoltMod::Menu>(VoltMod::PlayerRef target)> open);

}  // namespace AdminSystem::Admin::Menu
