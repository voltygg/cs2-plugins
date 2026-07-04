#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Menu/Menu.hpp>
#include <memory>

namespace AdminSystem::Admin::Menu
{

std::shared_ptr<CS2Kit::MenuView> BuildEffectsMenu(int adminSlot);
std::shared_ptr<CS2Kit::MenuView> BuildEffectsActionsMenu(int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Menu
