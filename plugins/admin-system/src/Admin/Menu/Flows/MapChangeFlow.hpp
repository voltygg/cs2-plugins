#pragma once

#include "Admin/Menu/MenuContext.hpp"
#include "Maps/MapQuery.hpp"

namespace AdminSystem::Admin::Menu
{

/** Ask before switching level: changing the map takes the server away from everyone on it. */
void ConfirmMapChange(const MenuContext& ctx, Maps::MapEntry map);

}  // namespace AdminSystem::Admin::Menu
