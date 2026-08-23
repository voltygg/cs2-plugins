#pragma once

// AimlockCore's only engine dependency; everything else arrives through the ShotCorrelator feed.

#include "Detectors/AimlockCore.hpp"

namespace Anticheat
{

/**
 * Visual lag for @p slot from its channel round trip and its replicated cl_interp_ratio. Invalid
 * without a live channel or a numeric interp value: a guessed lag would let the module test
 * hypotheses the client never held.
 */
LagEstimate MeasureVisualLag(CS2Kit::Runtime& rt, int slot);

}  // namespace Anticheat
