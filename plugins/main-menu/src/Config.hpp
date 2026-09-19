#pragma once

#include <VoltMod/App/Config.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace MainMenu
{

/** One row of a tab. `kind` says which of the target fields it reads. */
struct EntrySettings
{
    /** "command", "link" or "section". */
    std::string kind;
    /** A translation key, or literal text when no translation has that key. */
    std::string label;
    /** For "command": a console command run as the player, such as `mm_lvl`. */
    std::string command;
    /** For "link": printed to the player's chat; a server cannot open a browser. */
    std::string url;
    /** For "section": the id another plugin publishes a Contracts::IMenuSection under. */
    std::string section;
};

struct TabSettings
{
    std::string label;
    /** One of the names in panorama/screens/main_menu/icons.j2. */
    std::string icon;
    std::vector<EntrySettings> entries;
};

struct MenuSettings
{
    /** Without it, or without the layout on the client, the menu is center HTML. */
    bool panorama = true;
    /** Workshop addon carrying the compiled layout; 0 requires nothing. */
    uint64_t addonId = 0;
};

struct Settings
{
    VoltMod::StandardPluginSettings plugin;
    MenuSettings menu;
    std::vector<TabSettings> tabs;
};

/** The layout draws no more tabs than this. */
inline constexpr int MaxTabs = 6;

/** Drops entries with an unknown kind or an unusable target, and tabs past MaxTabs, logging each. */
Settings CleanSettings(Settings raw);

using ConfigManager = VoltMod::Options<Settings, Settings>;

}  // namespace MainMenu
