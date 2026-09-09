#pragma once

#include <cstdint>

namespace AdminSystem::Config
{

/** The `menu` object in settings.jsonc. */
struct MenuSettings
{
    /**
     * Draw the clickable Panorama menu instead of center HTML.
     *
     * Needs the compiled `admin_menu` layout on the client, which arrives one of two ways:
     * `voltmod panorama compile` installs it into your own client for testing, and @ref addonId
     * ships it to everyone else. Drawing into a layout a client does not have shows nothing.
     */
    bool panorama = false;

    /**
     * Workshop addon carrying the compiled layout, required of every connecting client.
     *
     * 0 requires nothing, which is what you want while testing against a client you compiled
     * into by hand. On a live server leaving it 0 means anyone who has not copied the files in
     * sees an empty menu, so set it before turning @ref panorama on for real players.
     */
    uint64_t addonId = 0;
};

}  // namespace AdminSystem::Config
