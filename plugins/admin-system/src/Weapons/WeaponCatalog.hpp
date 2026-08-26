#pragma once

#include <string>

namespace AdminSystem::Weapons
{

/** One giveable weapon. @ref Item is the entity classname the engine knows, e.g. "weapon_ak47". */
struct WeaponEntry
{
    std::string Name;
    std::string Item;

    const std::string& Label() const { return Name.empty() ? Item : Name; }
};

/**
 * Why @p entry cannot be offered, or an empty string when it is fine.
 *
 * Kept free of the framework and the SDK so it is unit-testable.
 */
std::string ValidateWeaponEntry(const WeaponEntry& entry);

}  // namespace AdminSystem::Weapons
