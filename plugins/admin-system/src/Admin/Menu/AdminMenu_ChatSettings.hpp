#pragma once

#include "../../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

/** Build the per-admin chat settings submenu (prefix toggle + prefix/name/message colors). */
std::shared_ptr<VoltMod::MenuView> BuildChatSettingsMenu(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin::Menu
