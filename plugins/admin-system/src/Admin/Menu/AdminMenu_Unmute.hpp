#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Combined list of active voice/text mutes; selecting one opens an unmute confirmation dialog. */
std::shared_ptr<CS2Kit::MenuView> BuildUnmuteMenu(int adminSlot);

}  // namespace AdminSystem::Admin::Menu
