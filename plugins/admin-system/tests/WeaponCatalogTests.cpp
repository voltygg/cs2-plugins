#include "Weapons/WeaponCatalog.hpp"

#include <doctest/doctest.h>

using AdminSystem::Weapons::FindWeapon;
using AdminSystem::Weapons::ValidateWeaponEntry;
using AdminSystem::Weapons::WeaponEntry;
using AdminSystem::Weapons::WeaponMatch;

namespace
{

std::vector<WeaponEntry> Menu()
{
    return {
        {.Name = "AK-47", .Item = "weapon_ak47"},          {.Name = "AWP", .Item = "weapon_awp"},
        {.Name = "Desert Eagle", .Item = "weapon_deagle"}, {.Name = "Knife", .Item = "weapon_knife"},
        {.Name = "Flashbang", .Item = "weapon_flashbang"},
    };
}

}  // namespace

TEST_CASE("FindWeapon matches a classname with or without the weapon prefix")
{
    auto bare = FindWeapon(Menu(), "ak47");
    REQUIRE(bare.Result == WeaponMatch::Unique);
    CHECK_EQ(Menu()[bare.Index].Item, std::string("weapon_ak47"));

    auto full = FindWeapon(Menu(), "weapon_ak47");
    REQUIRE(full.Result == WeaponMatch::Unique);
    CHECK_EQ(Menu()[full.Index].Item, std::string("weapon_ak47"));
}

TEST_CASE("FindWeapon matches the display name so an operator label is typeable")
{
    auto hit = FindWeapon(Menu(), "Desert Eagle");
    REQUIRE(hit.Result == WeaponMatch::Unique);
    CHECK_EQ(Menu()[hit.Index].Item, std::string("weapon_deagle"));
}

TEST_CASE("FindWeapon ignores case")
{
    CHECK(FindWeapon(Menu(), "AWP").Result == WeaponMatch::Unique);
    CHECK(FindWeapon(Menu(), "awp").Result == WeaponMatch::Unique);
}

TEST_CASE("FindWeapon falls back to prefix then substring")
{
    CHECK(FindWeapon(Menu(), "deag").Result == WeaponMatch::Unique);
    CHECK(FindWeapon(Menu(), "bang").Result == WeaponMatch::Unique);
}

TEST_CASE("FindWeapon reports ambiguity instead of guessing")
{
    std::vector<WeaponEntry> menu{{.Name = "M4A4", .Item = "weapon_m4a1"},
                                  {.Name = "M4A1-S", .Item = "weapon_m4a1_silencer"}};
    // Prefixes both classnames and both display names, and nothing matches exactly.
    auto hit = FindWeapon(menu, "m4");
    CHECK(hit.Result == WeaponMatch::Ambiguous);
    CHECK_EQ(hit.Count, 2u);
}

TEST_CASE("FindWeapon prefers an exact classname over a longer sibling")
{
    std::vector<WeaponEntry> menu{{.Name = "M4A4", .Item = "weapon_m4a1"},
                                  {.Name = "M4A1-S", .Item = "weapon_m4a1_silencer"}};
    auto hit = FindWeapon(menu, "m4a1");
    REQUIRE(hit.Result == WeaponMatch::Unique);
    CHECK_EQ(menu[hit.Index].Item, std::string("weapon_m4a1"));
}

TEST_CASE("FindWeapon reports no match for an unknown or empty query")
{
    CHECK(FindWeapon(Menu(), "rocket_launcher").Result == WeaponMatch::None);
    CHECK(FindWeapon(Menu(), "").Result == WeaponMatch::None);
    CHECK(FindWeapon(Menu(), "weapon_").Result == WeaponMatch::None);
    CHECK(FindWeapon({}, "ak47").Result == WeaponMatch::None);
}

TEST_CASE("ValidateWeaponEntry requires a weapon classname")
{
    CHECK(ValidateWeaponEntry({.Name = "AK-47", .Item = "weapon_ak47"}).empty());
    CHECK_FALSE(ValidateWeaponEntry({.Name = "AK-47", .Item = ""}).empty());
    // Anything else would be handed to GiveNamedItem as an arbitrary entity classname.
    CHECK_FALSE(ValidateWeaponEntry({.Name = "Chicken", .Item = "chicken"}).empty());
}

TEST_CASE("WeaponEntry labels fall back to the classname")
{
    CHECK_EQ(WeaponEntry{.Item = "weapon_ak47"}.Label(), std::string("weapon_ak47"));
    CHECK_EQ(WeaponEntry{.Name = "AK-47", .Item = "weapon_ak47"}.Label(), std::string("AK-47"));
}
