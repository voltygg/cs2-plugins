#pragma once

#include "Menu.hpp"
#include <string>

namespace AdminSystem::Menu {

std::string RenderMenuHtml(const Menu* menu, int selectedIndex);

// Default layout sections
std::string DefaultHeader(const std::string& title);
std::string DefaultFooter();

} // namespace AdminSystem::Menu
