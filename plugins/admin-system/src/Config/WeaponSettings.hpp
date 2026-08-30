#pragma once

#include <nlohmann/json.hpp>
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
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WeaponConfigEntry, name, item)

/** Weapons offered by `!give` and the weapon menu. Defaulted for the same reason as
 *  @ref MapSettings::cycle. */
struct WeaponSettings
{
    std::vector<WeaponConfigEntry> menu = {
        {"AK-47", "weapon_ak47"},
        {"M4A4", "weapon_m4a1"},
        {"AWP", "weapon_awp"},
        {"Desert Eagle", "weapon_deagle"},
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
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WeaponSettings, menu)

}  // namespace AdminSystem::Config
