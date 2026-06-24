#pragma once

#include "EffectAction.hpp"
#include "EffectId.hpp"

#include <iterator>

namespace AdminSystem::Admin::Effects
{

/** Toggle Disco: cycles bright render colors on a timer, auto-cancelling after a fixed duration. */
extern const EffectToggle Disco;

/** Toggle Ghost: invisible render. Re-applying cancels and restores visibility. */
extern const EffectToggle Ghost;

/**
 * Toggle stealth-spectator on a player: moves them to the spectator team in free-roam
 * observer mode and clears their scoreboard name. Re-applying restores the original team
 * and name. Self-only in practice — invoked via Run(adminSlot, adminSlot, Hide).
 */
extern const EffectToggle Hide;

/** Ordered to match the EffectId enum so adding an EffectId without a descriptor is a compile error. */
[[maybe_unused]] inline const EffectToggle* const EffectRegistry[] = {&Disco, &Ghost, &Hide};
static_assert(std::size(EffectRegistry) == static_cast<size_t>(EffectId::Count),
              "every EffectId needs a descriptor");

}  // namespace AdminSystem::Admin::Effects
