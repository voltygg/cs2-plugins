#include "WeaponClass.hpp"

#include <algorithm>
#include <array>

namespace Anticheat
{

namespace
{
// Deviation ceilings per weapon class, in degrees. Precision rifles sit lowest (their first shot is
// near-perfectly accurate), SMGs highest (spray plus movement inaccuracy is legitimately wide).
constexpr float RifleDeviation = 12.5f;
constexpr float SniperDeviation = 2.5f;
constexpr float HeavyPistolDeviation = 4.5f;
constexpr float PistolDeviation = 4.3f;
constexpr float SmgDeviation = 22.5f;
constexpr float HeavyDeviation = 13.5f;
constexpr float DefaultDeviation = 15.5f;

constexpr std::array<std::string_view, 35> BallisticWeapons = {
    "ak47",   "aug",     "awp",   "bizon",   "cz75a", "deagle", "elite",         "famas",    "fiveseven",
    "g3sg1",  "galilar", "glock", "hkp2000", "m249",  "m4a1",   "m4a1_silencer", "mac10",    "mag7",
    "mp5sd",  "mp7",     "mp9",   "negev",   "nova",  "p250",   "p90",           "revolver", "sawedoff",
    "scar20", "sg556",   "ssg08", "taser",   "tec9",  "ump45",  "usp_silencer",  "xm1014",
};

constexpr std::array<std::string_view, 9> Rifles = {"ak47", "m4a1",  "m4a1_silencer", "galilar", "famas",
                                                    "aug",  "sg556", "g3sg1",         "scar20"};
constexpr std::array<std::string_view, 2> Snipers = {"awp", "ssg08"};
constexpr std::array<std::string_view, 2> HeavyPistols = {"deagle", "revolver"};
constexpr std::array<std::string_view, 8> Pistols = {"glock", "hkp2000", "usp_silencer", "elite",
                                                     "p250",  "tec9",    "fiveseven",    "cz75a"};
constexpr std::array<std::string_view, 7> Smgs = {"mac10", "mp9", "mp7", "mp5sd", "ump45", "p90", "bizon"};
constexpr std::array<std::string_view, 6> Heavies = {"nova", "xm1014", "sawedoff", "mag7", "m249", "negev"};

template <size_t N>
bool Contains(const std::array<std::string_view, N>& set, std::string_view weapon)
{
    return std::find(set.begin(), set.end(), weapon) != set.end();
}
}  // namespace

std::string_view NormalizeWeapon(std::string_view weapon)
{
    constexpr std::string_view prefix = "weapon_";
    if (weapon.starts_with(prefix))
        weapon.remove_prefix(prefix.size());
    return weapon;
}

bool IsBallisticWeapon(std::string_view weapon)
{
    return Contains(BallisticWeapons, NormalizeWeapon(weapon));
}

float SilentAimDeviationThreshold(std::string_view weapon)
{
    weapon = NormalizeWeapon(weapon);
    if (Contains(Rifles, weapon))
        return RifleDeviation;
    if (Contains(Snipers, weapon))
        return SniperDeviation;
    if (Contains(HeavyPistols, weapon))
        return HeavyPistolDeviation;
    if (Contains(Pistols, weapon))
        return PistolDeviation;
    if (Contains(Smgs, weapon))
        return SmgDeviation;
    if (Contains(Heavies, weapon))
        return HeavyDeviation;
    return DefaultDeviation;
}

}  // namespace Anticheat
