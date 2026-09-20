#pragma once

#include <string>
#include <vector>

namespace AdminSystem::Config
{
/** Raw weapon entry, validated into Weapons::WeaponEntry during load. */
struct WeaponConfigEntry
{
    std::string name;
    std::string item;
};

/** Weapons offered by the weapon menu. Defaulted for the same reason as
 *  @ref MapSettings::cycle. */
struct WeaponSettings
{
    std::vector<WeaponConfigEntry> menu = {
        {"AK-47", "weapon_ak47"},
        {"M4A4", "weapon_m4a1"},
        {"AWP", "weapon_awp"},
        {"Desert Eagle", "weapon_deagle"},
        {"R8 Revolver", "weapon_revolver"},
        {"MP9", "weapon_mp9"},
        {"Nova", "weapon_nova"},
        {"Negev", "weapon_negev"},
        {"Knife", "weapon_knife"},
        {"HE Grenade", "weapon_hegrenade"},
        {"Flashbang", "weapon_flashbang"},
        {"Smoke", "weapon_smokegrenade"},
        {"Molotov", "weapon_molotov"},
    };
};

}  // namespace AdminSystem::Config
