#pragma once

#include "../Config.hpp"
#include "../PlayerMonitor.hpp"
#include "Detection.hpp"

#include <optional>

namespace Anticheat::Detectors::SilentAim
{

/**
 * Per-tick: a large viewangle jump on a command with ~zero mouse input means
 * software wrote the aim. Arms the confirmation window; scoring happens only
 * when on-target damage follows (legit flicks always have matching mouse deltas).
 */
void OnCmd(const SilentAimSettings& cfg, PlayerState& s, const CS2Kit::UserCmdView& cmd, float yawDelta,
           float pitchDelta);

/** On player_hurt from this attacker: confirm a pending mouse-less jump. */
std::optional<Detection> OnDamage(const SilentAimSettings& cfg, PlayerState& s, float aimErrorDeg, double now);

}  // namespace Anticheat::Detectors::SilentAim
