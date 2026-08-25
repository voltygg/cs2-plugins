#include "Maps/MapQuery.hpp"

#include <doctest/doctest.h>

using AdminSystem::Maps::FindMap;
using AdminSystem::Maps::MapEntry;
using AdminSystem::Maps::MapMatch;
using AdminSystem::Maps::ValidateMapEntry;

namespace
{

std::vector<MapEntry> Cycle()
{
    return {
        {.Name = "de_dust2", .DisplayName = "Dust II"},
        {.Name = "de_dust2_night"},
        {.Name = "de_mirage", .DisplayName = "Mirage"},
        {.Name = "cs_office"},
    };
}

}  // namespace

TEST_CASE("FindMap prefers an exact name over a longer one that starts with it")
{
    auto hit = FindMap(Cycle(), "de_dust2");
    REQUIRE(hit.Result == MapMatch::Unique);
    CHECK_EQ(Cycle()[hit.Index].Name, std::string("de_dust2"));
}

TEST_CASE("FindMap falls back to a unique prefix")
{
    auto hit = FindMap(Cycle(), "de_mir");
    REQUIRE(hit.Result == MapMatch::Unique);
    CHECK_EQ(Cycle()[hit.Index].Name, std::string("de_mirage"));
}

TEST_CASE("FindMap falls back to a unique substring")
{
    auto hit = FindMap(Cycle(), "office");
    REQUIRE(hit.Result == MapMatch::Unique);
    CHECK_EQ(Cycle()[hit.Index].Name, std::string("cs_office"));
}

TEST_CASE("FindMap reports ambiguity instead of guessing")
{
    // Both de_dust2 and de_dust2_night carry this prefix, and neither display name rescues it.
    auto hit = FindMap(Cycle(), "de_dust");
    CHECK(hit.Result == MapMatch::Ambiguous);
    CHECK_EQ(hit.Count, 2u);
}

TEST_CASE("FindMap lets a display name break an otherwise ambiguous query")
{
    // "dust" prefix-matches no engine name, but it does prefix "Dust II" - and only that one,
    // so the query resolves rather than making the admin type the full engine name.
    auto hit = FindMap(Cycle(), "dust");
    REQUIRE(hit.Result == MapMatch::Unique);
    CHECK_EQ(Cycle()[hit.Index].Name, std::string("de_dust2"));
}

TEST_CASE("FindMap matches the display name so an operator label is typeable")
{
    auto hit = FindMap(Cycle(), "Dust II");
    REQUIRE(hit.Result == MapMatch::Unique);
    CHECK_EQ(Cycle()[hit.Index].Name, std::string("de_dust2"));
}

TEST_CASE("FindMap ignores case")
{
    CHECK(FindMap(Cycle(), "DE_MIRAGE").Result == MapMatch::Unique);
    CHECK(FindMap(Cycle(), "mIrAgE").Result == MapMatch::Unique);
}

TEST_CASE("FindMap reports no match for an unknown or empty query")
{
    CHECK(FindMap(Cycle(), "de_train").Result == MapMatch::None);
    CHECK(FindMap(Cycle(), "").Result == MapMatch::None);
    CHECK(FindMap({}, "de_dust2").Result == MapMatch::None);
}

TEST_CASE("ValidateMapEntry accepts a plain and a workshop entry")
{
    CHECK(ValidateMapEntry({.Name = "de_dust2"}).empty());
    CHECK(ValidateMapEntry({.Name = "my_map", .WorkshopId = 3070563536ull}).empty());
}

TEST_CASE("ValidateMapEntry rejects an entry with no name")
{
    CHECK_FALSE(ValidateMapEntry({.Name = ""}).empty());
}

TEST_CASE("ValidateMapEntry rejects negative and inverted player limits")
{
    CHECK_FALSE(ValidateMapEntry({.Name = "de_dust2", .MinPlayers = -1}).empty());
    CHECK_FALSE(ValidateMapEntry({.Name = "de_dust2", .MinPlayers = 20, .MaxPlayers = 10}).empty());
}

TEST_CASE("ValidateMapEntry treats a zero maximum as no upper limit")
{
    // 0 must not read as "max below min" and disqualify an otherwise fine entry.
    CHECK(ValidateMapEntry({.Name = "de_dust2", .MinPlayers = 20, .MaxPlayers = 0}).empty());
}

TEST_CASE("MapEntry labels fall back to the engine name")
{
    CHECK_EQ(MapEntry{.Name = "de_nuke"}.Label(), std::string("de_nuke"));
    CHECK_EQ(MapEntry{.Name = "de_nuke", .DisplayName = "Nuke"}.Label(), std::string("Nuke"));
}
