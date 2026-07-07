#pragma once

#include "../Config.hpp"
#include "../PlayerMonitor.hpp"
#include "Detection.hpp"

#include <optional>

namespace Anticheat::Detectors::NoFlash
{

/** player_blind: remember until when this slot is blinded. */
void OnBlind(const NoFlashSettings& cfg, PlayerState& s, float blindDuration, double now);

/** A kill landed while still meaningfully blinded; repetition required for ban tier. */
std::optional<Detection> OnKill(const NoFlashSettings& cfg, PlayerState& s, bool headshot, double now);

}  // namespace Anticheat::Detectors::NoFlash
