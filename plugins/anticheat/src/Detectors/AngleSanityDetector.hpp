#pragma once

#include "../Config.hpp"
#include "../PlayerMonitor.hpp"
#include "Detection.hpp"

#include <optional>

namespace Anticheat::Detectors::AngleSanity
{

/** Per-tick: NaN/inf angles or pitch outside the legal range, sustained minTicks. */
std::optional<Detection> OnCmd(const SanitySettings& cfg, PlayerState& s, float pitch, float yaw);

}  // namespace Anticheat::Detectors::AngleSanity
