#include "MenuRenderer.hpp"

#include <sstream>

namespace AdminSystem::Menu {

// Gold & Ember theme colors (PascalCase constants)
namespace Theme {
    constexpr const char* Gold      = "#FFD700";
    constexpr const char* Amber     = "#FF8C00";
    constexpr const char* WarmWhite = "#CCBBAA";
    constexpr const char* WarmGray  = "#887755";
    constexpr const char* Disabled  = "#665544";
    constexpr const char* NavGold   = "#AA8833";
    constexpr const char* NavClose  = "#AA4422";
}

//-----------------------------------------------------------------------------
// Default header: top border + title + subtitle + divider
//-----------------------------------------------------------------------------

std::string DefaultHeader(const std::string& title)
{
    std::ostringstream html;

    // Top border
    html << "<font color='" << Theme::Gold << "'>* ===================== *</font><br>";

    // Title
    html << "<font color='" << Theme::Gold << "'><b>* "
         << title
         << " *</b></font><br>";

    // Subtitle
    html << "<font color='" << Theme::WarmGray << "'>admin-system v1.0</font><br>";

    // Divider
    html << "<font color='" << Theme::Gold << "'>=======================</font><br>";

    // Spacer
    html << "<br>";

    return html.str();
}

//-----------------------------------------------------------------------------
// Default footer: bottom border + navigation hints
//-----------------------------------------------------------------------------

std::string DefaultFooter()
{
    std::ostringstream html;

    // Spacer
    html << "<br>";

    // Bottom border
    html << "<font color='" << Theme::Gold << "'>=======================</font><br>";

    // Navigation hints
    html << "<font color='" << Theme::NavGold << "'>W/S Navigate</font>"
         << "   "
         << "<font color='" << Theme::Gold << "'>E Select</font>"
         << "   "
         << "<font color='" << Theme::NavClose << "'>R Close</font>";

    return html.str();
}

//-----------------------------------------------------------------------------
// Render menu items
//-----------------------------------------------------------------------------

static std::string RenderItems(const Menu* menu, int selectedIndex)
{
    std::ostringstream html;

    for (int i = 0; i < static_cast<int>(menu->Items.size()); ++i)
    {
        const auto& item = menu->Items[i];

        if (!item.Enabled)
        {
            // Disabled item
            html << "<font color='" << Theme::Disabled << "'>- "
                 << item.Title << "</font><br>";
        }
        else if (i == selectedIndex)
        {
            // Selected item - Gold & Amber highlight
            html << "<font color='" << Theme::Amber << "'><b>&gt; "
                 << item.Title << "</b></font> "
                 << "<font color='" << Theme::Gold << "'>[E]</font><br>";
        }
        else
        {
            // Normal item
            html << "<font color='" << Theme::WarmWhite << "'>  "
                 << item.Title << "</font><br>";
        }
    }

    return html.str();
}

//-----------------------------------------------------------------------------
// Full menu HTML rendering
//-----------------------------------------------------------------------------

std::string RenderMenuHtml(const Menu* menu, int selectedIndex)
{
    if (!menu)
        return "";

    std::ostringstream html;

    // Header: use custom if provided, otherwise default
    if (menu->Layout.Header)
    {
        html << menu->Layout.Header();
    }
    else
    {
        html << DefaultHeader(menu->Title);
    }

    // Items
    html << RenderItems(menu, selectedIndex);

    // Footer: use custom if provided, otherwise default
    if (menu->Layout.Footer)
    {
        html << menu->Layout.Footer();
    }
    else
    {
        html << DefaultFooter();
    }

    return html.str();
}

} // namespace AdminSystem::Menu
