#include "Maps/MapQuery.hpp"

#include <doctest/doctest.h>

using AdminSystem::Maps::MapEntry;
using AdminSystem::Maps::ValidateMapEntry;

TEST_CASE("ValidateMapEntry accepts a plain and a workshop entry")
{
    CHECK(ValidateMapEntry({.Name = "de_dust2"}).empty());
    CHECK(ValidateMapEntry({.Name = "my_map", .WorkshopId = 3070563536ull}).empty());
}

TEST_CASE("ValidateMapEntry rejects an entry with no name")
{
    CHECK_FALSE(ValidateMapEntry({.Name = ""}).empty());
}

TEST_CASE("MapEntry labels fall back to the engine name")
{
    CHECK_EQ(MapEntry{.Name = "de_nuke"}.Label(), std::string("de_nuke"));
    CHECK_EQ(MapEntry{.Name = "de_nuke", .DisplayName = "Nuke"}.Label(), std::string("Nuke"));
}
