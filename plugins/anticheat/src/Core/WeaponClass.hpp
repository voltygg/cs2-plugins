#pragma once

#include <string_view>

namespace Anticheat
{

/** Drops the "weapon_" prefix; returns @p weapon unchanged when it has none. */
std::string_view NormalizeWeapon(std::string_view weapon);

/**
 * True for weapons that fire a hitscan bullet along the command's aim angles. Grenades, the knife
 * and the bomb produce weapon_fire events that no shot correlation applies to, so they are out.
 */
bool IsBallisticWeapon(std::string_view weapon);

/**
 * Largest angle between a shot's visible aim and its impact point that is still explainable by
 * spread, recoil and lag for @p weapon. Anything above it is only reachable by writing the shot
 * angle separately from the view - the silent-aim signature.
 */
float SilentAimDeviationThreshold(std::string_view weapon);

}  // namespace Anticheat
