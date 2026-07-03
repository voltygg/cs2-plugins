#pragma once

#include "EffectAction.hpp"
#include "EffectId.hpp"

#include <iterator>

namespace AdminSystem::Admin::Effects
{

/** Toggle Disco: cycles bright render colors on a timer, auto-cancelling after a fixed duration. */
extern const EffectToggle Disco;

/** Toggle Ghost: full invisibility via transmit filtering (pawn + weapons + wearables). */
extern const EffectToggle Ghost;

/**
 * Toggle stealth-spectator on a player: moves them to the spectator team in free-roam
 * observer mode and clears their scoreboard name. Re-applying restores the original team
 * and name. Self-only in practice - invoked via Run(adminSlot, adminSlot, Hide).
 */
extern const EffectToggle Hide;

/** Toggle effects in EffectId order, so a new toggle without a descriptor is a compile error. Model
 *  is parameterized (Effects/Model.*), not a toggle, so it is excluded and the guard stops at Model. */
[[maybe_unused]] inline const EffectToggle* const EffectRegistry[] = {&Disco, &Ghost, &Hide};
static_assert(std::size(EffectRegistry) == static_cast<size_t>(EffectId::Model),
              "every toggle EffectId before Model needs a descriptor");

}  // namespace AdminSystem::Admin::Effects
