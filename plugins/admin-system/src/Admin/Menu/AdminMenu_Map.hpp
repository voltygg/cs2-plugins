#pragma once

#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Admin::Menu
{

/** Map picker: every configured map, each confirming before it takes the server away. */
std::shared_ptr<VoltMod::MenuView> BuildMapMenu(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin::Menu
