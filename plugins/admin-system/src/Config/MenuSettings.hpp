#pragma once

#include <cstdint>

namespace AdminSystem::Config
{

/** The `menu` object in settings.jsonc. */
struct MenuSettings
{
    /**
     * Workshop addon carrying the compiled admin_menu layout.
     *
     * 0 draws every admin the center-HTML menu, which needs nothing on the client. Set it and a
     * player who has finished downloading the addon gets the clickable Panorama menu instead;
     * without a required addon there is no way to know the client has the layout, and drawing
     * into one it does not have shows nothing at all.
     */
    uint64_t addonId = 0;
};

}  // namespace AdminSystem::Config
