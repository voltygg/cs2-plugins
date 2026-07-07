#pragma once

#include "../Config.hpp"
#include "../PlayerMonitor.hpp"
#include "Detection.hpp"

#include <optional>

namespace Anticheat::Detectors::Spinbot
{

/**
 * Per-tick spin-state update. Spinning alone is only ever an observe-tier
 * signal (legit players fake-spin); it arms the kill-correlation window.
 */
std::optional<Detection> OnCmd(const SpinSettings& cfg, PlayerState& s, float yawDelta);

/** True when the slot was in spin state within the last killWindowTicks. */
bool RecentlySpinning(const SpinSettings& cfg, const PlayerState& s);

/**
 * A kill landed while (or right after) spinning. Only on-target fire-tick aim
 * counts as a spin-snap-kill event; repetition is required for ban tier.
 */
std::optional<Detection> OnKill(const SpinSettings& cfg, PlayerState& s, bool headshot, float aimErrorDeg, double now);

}  // namespace Anticheat::Detectors::Spinbot
