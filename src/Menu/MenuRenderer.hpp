#pragma once

#include "Menu.hpp"
#include <string>

namespace AdminSystem::Menu {

/**
 * Render a full menu to an HTML string (header + items + footer).
 * Uses the menu's custom Layout if set, otherwise falls back to the Gold & Ember theme.
 */
std::string RenderMenuHtml(const Menu* menu, int selectedIndex);

/** Default Gold & Ember themed header with the menu title. */
std::string DefaultHeader(const std::string& title);
/** Default footer with navigation hints (W/S, E, R). */
std::string DefaultFooter();

} // namespace AdminSystem::Menu
