#include "Config.hpp"

#include <cstddef>
#include <doctest/doctest.h>
#include <utility>
#include <vector>

using MainMenu::CleanSettings;
using MainMenu::EntrySettings;
using MainMenu::MaxTabs;
using MainMenu::Settings;
using MainMenu::TabSettings;

static Settings WithEntries(std::vector<EntrySettings> entries)
{
    Settings settings;
    settings.tabs.push_back(TabSettings{.label = "tab", .icon = "stats", .entries = std::move(entries)});
    return settings;
}

TEST_CASE("Usable entries of every kind are kept")
{
    const Settings clean = CleanSettings(WithEntries({
        {.kind = "command", .label = "level", .command = "mm_lvl"},
        {.kind = "link", .label = "rules", .url = "https://meat.gg/rules"},
        {.kind = "section", .label = "admin", .section = "admin"},
    }));
    CHECK(clean.tabs[0].entries.size() == 3);
}

TEST_CASE("Entries that cannot run are dropped")
{
    const Settings clean = CleanSettings(WithEntries({
        {.kind = "command", .label = "chained", .command = "mm_lvl; quit"},
        {.kind = "command", .label = "empty"},
        {.kind = "link", .label = "no scheme", .url = "meat.gg/rules"},
        {.kind = "section", .label = "no id"},
        {.kind = "teleport", .label = "unknown kind", .command = "mm_lvl"},
        {.kind = "command", .command = "mm_lvl"},
    }));
    CHECK(clean.tabs[0].entries.empty());
}

TEST_CASE("Tabs past the sidebar's size are dropped")
{
    Settings settings;
    settings.tabs.resize(static_cast<std::size_t>(MaxTabs) + 2);
    CHECK(CleanSettings(settings).tabs.size() == static_cast<std::size_t>(MaxTabs));
}
