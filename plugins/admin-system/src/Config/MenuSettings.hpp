#pragma once

#include <cstdint>
#include <string>

namespace AdminSystem::Config
{
/**
 * Which menu host the plugin opens its menus on.
 *
 * `Panorama` is `Auto` under another name: neither can be honoured without
 * VoltMod::Capability::CustomUi (today, any non-Windows server), and both fall back to center HTML
 * rather than show nobody a menu. `CenterHtml` pins the plain menu on every build.
 */
enum class MenuStyle
{
    Auto,
    Panorama,
    CenterHtml,
};

struct MenuSettings
{
    /** `auto` | `panorama` | `centerHtml`; anything else warns at load and is read as `auto`. */
    std::string style = "auto";
    /** Layout resource to drive; must honour the ids VoltMod's own layout declares. */
    std::string layout = "voltmod_menu";
    /** Workshop addon carrying the compiled layout. 0 requires nothing of connecting clients,
     *  which is what you want while copying the files into your own client by hand. */
    uint64_t addonId = 0;
};

}  // namespace AdminSystem::Config
