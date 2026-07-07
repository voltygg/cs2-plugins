#pragma once

#include "../Config.hpp"
#include "../PlayerMonitor.hpp"
#include "Detection.hpp"

#include <optional>

namespace Anticheat::Detectors::AimSnap
{

/**
 * On weapon_fire: scan the slot's recent usercmd history for a large flick
 * that settled before the shot. Unconfirmed snaps only log (legit flicks
 * exist) and arm the damage-confirmation window.
 */
std::optional<Detection> OnFire(const AimSnapSettings& cfg, PlayerState& s, int slot);

/** On player_hurt from this attacker: confirm a pending snap when on-target. */
std::optional<Detection> OnDamage(const AimSnapSettings& cfg, PlayerState& s, float aimErrorDeg, double now);

}  // namespace Anticheat::Detectors::AimSnap
