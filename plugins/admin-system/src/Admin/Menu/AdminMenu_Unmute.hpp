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

/** Combined list of active voice/text mutes; selecting one opens an unmute confirmation dialog. */
std::shared_ptr<VoltMod::MenuView> BuildUnmuteMenu(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin::Menu
