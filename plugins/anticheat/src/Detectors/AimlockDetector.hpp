#pragma once

// AimlockCore's only engine dependency: how far in the past the world a client is aiming at
// actually is. Everything else the module needs already arrives through the ShotCorrelator feed.

#include "Detectors/AimlockCore.hpp"

namespace Anticheat
{

/**
 * Visual lag for @p slot from its channel round trip and its replicated cl_interp_ratio. Invalid
 * (and therefore never evidence) for a slot with no live channel or a non-numeric interp value -
 * a guessed lag would let the module test hypotheses the client never held.
 */
LagEstimate MeasureVisualLag(int slot);

}  // namespace Anticheat
