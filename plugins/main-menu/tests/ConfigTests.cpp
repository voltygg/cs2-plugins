#include "Config.hpp"

#include <cstddef>
#include <doctest/doctest.h>
#include <utility>
#include <vector>

using MainMenu::CleanSettings;
using MainMenu::EntryKind;
using MainMenu::EntrySettings;
using MainMenu::Hub;
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
    const Hub clean = CleanSettings(WithEntries({
        {.kind = "command", .label = "level", .command = "mm_lvl"},
        {.kind = "link", .label = "rules", .url = "https://meat.gg/rules"},
        {.kind = "section", .label = "admin", .section = "admin"},
    }));
    REQUIRE(clean.tabs[0].entries.size() == 3);
    CHECK(clean.tabs[0].entries[0].kind == EntryKind::Command);
    CHECK(clean.tabs[0].entries[0].target == "mm_lvl");
    CHECK(clean.tabs[0].entries[1].kind == EntryKind::Link);
    CHECK(clean.tabs[0].entries[1].target == "https://meat.gg/rules");
    CHECK(clean.tabs[0].entries[2].kind == EntryKind::Section);
    CHECK(clean.tabs[0].entries[2].target == "admin");
}

TEST_CASE("Panorama is on unless the settings turn it off")
{
    CHECK(CleanSettings(Settings{}).menu.panorama);
}

TEST_CASE("Entries that cannot run are dropped")
{
    const Hub clean = CleanSettings(WithEntries({
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
