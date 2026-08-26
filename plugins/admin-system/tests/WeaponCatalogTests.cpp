#include "Weapons/WeaponCatalog.hpp"

#include <doctest/doctest.h>

using AdminSystem::Weapons::ValidateWeaponEntry;
using AdminSystem::Weapons::WeaponEntry;

TEST_CASE("ValidateWeaponEntry accepts an entity classname")
{
    CHECK(ValidateWeaponEntry({.Name = "AK-47", .Item = "weapon_ak47"}).empty());
}

TEST_CASE("ValidateWeaponEntry rejects an entry with no item")
{
    CHECK_FALSE(ValidateWeaponEntry({.Name = "AK-47", .Item = ""}).empty());
}

TEST_CASE("ValidateWeaponEntry rejects anything that is not a weapon classname")
{
    // Otherwise the string reaches GiveNamedItem as an arbitrary entity.
    CHECK_FALSE(ValidateWeaponEntry({.Name = "Chicken", .Item = "chicken"}).empty());
    CHECK_FALSE(ValidateWeaponEntry({.Name = "Bomb", .Item = "planted_c4"}).empty());
}

TEST_CASE("WeaponEntry labels fall back to the classname")
{
    CHECK_EQ(WeaponEntry{.Item = "weapon_ak47"}.Label(), std::string("weapon_ak47"));
    CHECK_EQ(WeaponEntry{.Name = "AK-47", .Item = "weapon_ak47"}.Label(), std::string("AK-47"));
}
