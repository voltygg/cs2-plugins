#pragma once

#include "../Config.hpp"
#include "../PlayerMonitor.hpp"
#include "Detection.hpp"

#include <CS2Kit/Sdk/UserCmd.hpp>
#include <optional>

namespace Anticheat::Detectors::ShotAngle
{

/**
 * Angular divergence between the angle the bullet was fired along (input_history) and the visible
 * view angle, or nullopt when this command started no shot or carries no fired angles.
 */
std::optional<float> ShotDivergence(const CS2Kit::Sdk::UserCmdView& cmd);

/**
 * On a command that fired: the shot's view angle (input_history) vs the visible view angle.
 * They match on a legit client; divergence means software wrote the shot. Arms confirmation.
 */
void OnCmd(const ShotAngleSettings& cfg, PlayerState& s, const CS2Kit::Sdk::UserCmdView& cmd);

/** On player_hurt from this attacker: a diverged shot that connected is a confirmed event. */
std::optional<Detection> OnDamage(const ShotAngleSettings& cfg, PlayerState& s, double now);

}  // namespace Anticheat::Detectors::ShotAngle
