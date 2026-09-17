#pragma once

#include <string_view>

namespace Anticheat
{

/** Drops the "weapon_" prefix; returns @p weapon unchanged when it has none. */
std::string_view NormalizeWeapon(std::string_view weapon);

/**
 * True for weapons that fire a hitscan bullet along the command's aim angles. Grenades, the knife
 * and the bomb also produce weapon_fire events, but no shot correlation applies to them.
 */
bool IsBallisticWeapon(std::string_view weapon);

/**
 * Largest angle between a shot's visible aim and its impact point still explainable by spread,
 * recoil and lag for @p weapon. Above it, the shot angle was written separately from the view.
 */
float SilentAimDeviationThreshold(std::string_view weapon);

}  // namespace Anticheat
