#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Admin::Menu
{

/** Build the per-admin chat settings submenu (prefix toggle + prefix/name/message colors). */
std::shared_ptr<CS2Kit::MenuView> BuildChatSettingsMenu(AdminSystem::App& app, int adminSlot);

}  // namespace AdminSystem::Admin::Menu
