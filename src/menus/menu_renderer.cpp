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
    html << "<font color='" << theme::GOLD << "'>* ===================== *</font><br>";

    // Title
    html << "<font color='" << theme::GOLD << "'><b>* "
         << menu->title
         << " *</b></font><br>";

    // Subtitle
    html << "<font color='" << theme::WARM_GRAY << "'>admin-system v1.0</font><br>";

    // Divider
    html << "<font color='" << theme::GOLD << "'>=======================</font><br>";

    // Spacer
    html << "<br>";

    // Menu items
    for (int i = 0; i < static_cast<int>(menu->items.size()); ++i)
    {
        const auto& item = menu->items[i];

        if (!item.enabled)
        {
            // Disabled item
            html << "<font color='" << theme::DISABLED << "'>- "
                 << item.title << "</font><br>";
        }
        else if (i == selected_index)
        {
            // Selected item - Gold & Amber highlight
            html << "<font color='" << theme::AMBER << "'><b>&gt; "
                 << item.title << "</b></font> "
                 << "<font color='" << theme::GOLD << "'>[E]</font><br>";
        }
        else
        {
            // Normal item
            html << "<font color='" << theme::WARM_WHITE << "'>  "
                 << item.title << "</font><br>";
        }
    }

    // Spacer
    html << "<br>";

    // Bottom border
    html << "<font color='" << theme::GOLD << "'>=======================</font><br>";

    // Navigation hints
    html << "<font color='" << theme::NAV_GOLD << "'>W/S Navigate</font>"
         << "   "
         << "<font color='" << theme::GOLD << "'>E Select</font>"
         << "   "
         << "<font color='" << theme::NAV_CLOSE << "'>R Close</font>";

    return html.str();
}

} // namespace menus
