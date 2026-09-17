#pragma once

#include "Detect/ViewLag.hpp"

#include <VoltMod/Api.hpp>

namespace Anticheat
{

/**
 * Visual lag for @p slot from its channel round trip and its replicated cl_interp_ratio. Invalid
 * without a live channel or a numeric interp value: a guessed lag would let the detectors test
 * hypotheses the client never held.
 */
ViewLag MeasureViewLag(VoltMod::Runtime& rt, int slot);

}  // namespace Anticheat
