#pragma once

#include <string_view>

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Weapons
{

/** Why a give or strip did not happen, for a caller that wants to say so. */
enum class WeaponActionResult
{
    Ok,
    NotAllowed,  ///< permission or immunity refused the target; the dispatcher already replied
    TargetDead,
    EngineRefused,
};

/**
 * Give @p item to @p targetSlot, or strip everything they carry, and broadcast the action.
 *
 * The single place the resolve/alive/broadcast sequence for weapon actions lives, so the chat
 * commands and the admin menu cannot drift on the permission flag or the broadcast key.
 * Permission and immunity are re-resolved on every call, since a menu may have been open a while.
 */
WeaponActionResult GiveWeapon(App& app, int adminSlot, int targetSlot, std::string_view item);
WeaponActionResult StripWeapons(App& app, int adminSlot, int targetSlot);

}  // namespace AdminSystem::Weapons
