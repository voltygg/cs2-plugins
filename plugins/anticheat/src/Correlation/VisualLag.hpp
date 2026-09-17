#pragma once

// The aim cores' only network dependency: how stale the world a client aimed at was.

#include "Core/LagEstimate.hpp"

#include <VoltMod/Api.hpp>

namespace Anticheat
{

/**
 * Visual lag for @p slot from its channel round trip and its replicated cl_interp_ratio. Invalid
 * without a live channel or a numeric interp value: a guessed lag would let the cores test
 * hypotheses the client never held.
 */
LagEstimate MeasureVisualLag(VoltMod::Runtime& rt, int slot);

}  // namespace Anticheat
