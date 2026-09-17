#pragma once

// How far in the past the world a client aimed at actually is. SDK-free: the engine adapter in
// Correlation/VisualLag.cpp measures the inputs, every aim core consumes the estimate.

#include "Core/Samples.hpp"

#include <algorithm>
#include <cmath>

namespace Anticheat
{

/** How far in the past the world the client aimed at actually is, in server ticks. */
struct LagEstimate
{
    int Ticks = 0;
    float RoundTripMs = 0.0f;
    float InterpTicks = 0.0f;
    bool Valid = false;
};

/** The cores test this many ticks either side of the estimate, so interpolation jitter cannot
 *  make a true lag fall between two hypotheses. */
inline constexpr int LagSearchRadius = 2;
inline constexpr int LagHypothesisCount = 2 * LagSearchRadius + 1;
inline constexpr float MaximumInterpolationTicks = 19.0f;

/**
 * The snapshot travelled to the client before this command travelled back, so the round trip plus
 * the interpolation delay is the age of what the player saw. Invalid - and therefore never evidence
 * - for absurd RTT or cl_interp_ratio values.
 */
inline LagEstimate EstimateVisualLag(float rttSeconds, float interpRatio)
{
    LagEstimate estimate;
    if (!std::isfinite(rttSeconds) || rttSeconds < 0.0f || rttSeconds > 2.0f || !std::isfinite(interpRatio) ||
        interpRatio < 0.0f || interpRatio > MaximumInterpolationTicks)
        return estimate;

    const float interpTicks = interpRatio == 0.0f ? 1.0f : interpRatio;
    estimate.Ticks = std::max(0, static_cast<int>(std::lround(rttSeconds * TickRate + interpTicks)));
    estimate.RoundTripMs = rttSeconds * 1000.0f;
    estimate.InterpTicks = interpTicks;
    estimate.Valid = true;
    return estimate;
}

/** The lag hypothesis at @p index, clamped at zero: the client cannot see the future. */
inline int LagHypothesis(const LagEstimate& lag, int index)
{
    return std::max(0, lag.Ticks - LagSearchRadius + index);
}

}  // namespace Anticheat
