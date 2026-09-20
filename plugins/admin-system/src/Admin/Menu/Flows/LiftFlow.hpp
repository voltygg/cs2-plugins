#pragma once

#include "Admin/Menu/MenuContext.hpp"

#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Every active ban and mute this admin may lift; selecting one opens a confirmation. */
std::shared_ptr<VoltMod::Menu> BuildLiftMenu(const MenuContext& ctx);

}  // namespace AdminSystem::Admin::Menu
