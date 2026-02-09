#include "menu_renderer.h"

#include <format>
#include <sstream>

namespace menus {

// Gold & Ember theme colors
namespace theme {
    constexpr const char* GOLD       = "#FFD700";
    constexpr const char* AMBER      = "#FF8C00";
    constexpr const char* WARM_WHITE = "#CCBBAA";
    constexpr const char* WARM_GRAY  = "#887755";
    constexpr const char* DISABLED   = "#665544";
    constexpr const char* NAV_GOLD   = "#AA8833";
    constexpr const char* NAV_CLOSE  = "#AA4422";
}

std::string RenderMenuHtml(const Menu* menu, int selected_index)
{
    if (!menu)
        return "";

    std::ostringstream html;

    // Top border
    html << "<font color='" << theme::GOLD << "'>\u2605 \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550 \u2605</font><br>";

    // Title
    html << "<font color='" << theme::GOLD << "'><b>\u2605 "
         << menu->title
         << " \u2605</b></font><br>";

    // Subtitle
    html << "<font color='" << theme::WARM_GRAY << "'>admin-system v1.0</font><br>";

    // Divider
    html << "<font color='" << theme::GOLD << "'>\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550</font><br>";

    // Spacer
    html << "<br>";

    // Menu items
    for (int i = 0; i < static_cast<int>(menu->items.size()); ++i)
    {
        const auto& item = menu->items[i];

        if (!item.enabled)
        {
            // Disabled item
            html << "<font color='" << theme::DISABLED << "'>\u2500 "
                 << item.title << "</font><br>";
        }
        else if (i == selected_index)
        {
            // Selected item - Gold & Amber highlight
            html << "<font color='" << theme::AMBER << "'><b>\u2605 "
                 << item.title << "</b></font> "
                 << "<font color='" << theme::GOLD << "'>[E]</font><br>";
        }
        else
        {
            // Normal item
            html << "<font color='" << theme::WARM_WHITE << "'>\u2606 "
                 << item.title << "</font><br>";
        }
    }

    // Spacer
    html << "<br>";

    // Bottom border
    html << "<font color='" << theme::GOLD << "'>\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550</font><br>";

    // Navigation hints
    html << "<font color='" << theme::NAV_GOLD << "'>W \u25b2  S \u25bc</font>"
         << "          "
         << "<font color='" << theme::NAV_CLOSE << "'>R \u2715</font>";

    return html.str();
}

} // namespace menus
