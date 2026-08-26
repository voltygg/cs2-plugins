#include "WeaponClass.hpp"

#include <algorithm>
#include <array>

namespace Anticheat
{

// Deviation ceilings in degrees. Precision rifles sit lowest (their first shot is near-perfectly
// accurate), SMGs highest (spray plus movement inaccuracy is legitimately wide). One table, so a
// weapon cannot be ballistic without a ceiling or carry a ceiling without being ballistic.
static constexpr float RifleDeviation = 12.5f;
static constexpr float SniperDeviation = 2.5f;
static constexpr float HeavyPistolDeviation = 4.5f;
static constexpr float PistolDeviation = 4.3f;
static constexpr float SmgDeviation = 22.5f;
static constexpr float HeavyDeviation = 13.5f;
static constexpr float DefaultDeviation = 15.5f;

struct BallisticWeapon
{
    std::string_view Name;
    float DeviationCeiling;
};

static constexpr std::array BallisticWeapons = std::to_array<BallisticWeapon>({
    {"ak47", RifleDeviation},
    {"aug", RifleDeviation},
    {"famas", RifleDeviation},
    {"g3sg1", RifleDeviation},
    {"galilar", RifleDeviation},
    {"m4a1", RifleDeviation},
    {"m4a1_silencer", RifleDeviation},
    {"scar20", RifleDeviation},
    {"sg556", RifleDeviation},

    {"awp", SniperDeviation},
    {"ssg08", SniperDeviation},

    {"deagle", HeavyPistolDeviation},
    {"revolver", HeavyPistolDeviation},

    {"cz75a", PistolDeviation},
    {"elite", PistolDeviation},
    {"fiveseven", PistolDeviation},
    {"glock", PistolDeviation},
    {"hkp2000", PistolDeviation},
    {"p250", PistolDeviation},
    {"tec9", PistolDeviation},
    {"usp_silencer", PistolDeviation},

    {"bizon", SmgDeviation},
    {"mac10", SmgDeviation},
    {"mp5sd", SmgDeviation},
    {"mp7", SmgDeviation},
    {"mp9", SmgDeviation},
    {"p90", SmgDeviation},
    {"ump45", SmgDeviation},

    {"m249", HeavyDeviation},
    {"mag7", HeavyDeviation},
    {"negev", HeavyDeviation},
    {"nova", HeavyDeviation},
    {"sawedoff", HeavyDeviation},
    {"xm1014", HeavyDeviation},

    // Hitscan like the rest, but nothing about its spread resembles a bullet.
    {"taser", DefaultDeviation},
});

static const BallisticWeapon* Find(std::string_view weapon)
{
    auto found = std::find_if(BallisticWeapons.begin(), BallisticWeapons.end(),
                              [&](const BallisticWeapon& entry) { return entry.Name == weapon; });
    return found == BallisticWeapons.end() ? nullptr : &*found;
}

std::string_view NormalizeWeapon(std::string_view weapon)
{
    constexpr std::string_view prefix = "weapon_";
    if (weapon.starts_with(prefix))
        weapon.remove_prefix(prefix.size());
    return weapon;
}

bool IsBallisticWeapon(std::string_view weapon)
{
    return Find(NormalizeWeapon(weapon)) != nullptr;
}

float SilentAimDeviationThreshold(std::string_view weapon)
{
    const BallisticWeapon* found = Find(NormalizeWeapon(weapon));
    return found ? found->DeviationCeiling : DefaultDeviation;
}

}  // namespace Anticheat
