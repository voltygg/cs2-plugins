#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace AdminSystem::Weapons
{

/** One giveable weapon. @ref Item is the entity classname the engine knows, e.g. "weapon_ak47". */
struct WeaponEntry
{
    std::string Name;
    std::string Item;

    const std::string& Label() const { return Name.empty() ? Item : Name; }
};

enum class WeaponMatch
{
    None,
    Unique,
    Ambiguous,
};

struct WeaponLookup
{
    WeaponMatch Result = WeaponMatch::None;
    std::size_t Index = 0; /**< only meaningful when Result is Unique */
    std::size_t Count = 0;
};

/**
 * Resolve a typed weapon name against the configured list.
 *
 * Tiered like map and player lookups (exact, then prefix, then substring, case-insensitive) and
 * matches the display name as well as the classname, so both `!give ak47` and `!give AK-47`
 * work. The bare `weapon_` prefix is ignored on the query, since typing it is optional.
 *
 * Kept free of the framework and the SDK so it is unit-testable.
 */
WeaponLookup FindWeapon(const std::vector<WeaponEntry>& weapons, std::string_view query);

/** Why @p entry cannot be offered, or an empty string when it is fine. */
std::string ValidateWeaponEntry(const WeaponEntry& entry);

}  // namespace AdminSystem::Weapons
