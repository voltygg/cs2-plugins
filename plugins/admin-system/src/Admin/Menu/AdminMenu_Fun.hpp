#pragma once

#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Admin::Menu
{

/** Fun Mode: one toggle row per server-wide round modifier, plus a clear-all row. */
std::shared_ptr<VoltMod::MenuView> BuildFunMenu(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin::Menu
