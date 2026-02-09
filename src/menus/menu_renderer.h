#pragma once

#include "menu.h"
#include <string>

namespace menus {

/**
 * @brief Renders a menu to center-HTML string using Gold & Ember theme.
 * @param menu The menu to render
 * @param selected_index Currently selected item index
 * @return HTML string ready to send via SendCenterHtml
 */
std::string RenderMenuHtml(const Menu* menu, int selected_index);

} // namespace menus
