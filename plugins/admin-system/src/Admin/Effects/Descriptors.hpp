#pragma once

#include "../Actions/ActionContext.hpp"
#include "EffectId.hpp"

#include <CS2Kit/Api.hpp>

namespace AdminSystem::Admin::Effects
{

// The effect scaffold (descriptors + policy-checked dispatch) lives in the kit; these aliases
// keep descriptor files and call sites on the established local names. Dispatch with
// CS2Kit::ToggleEffect / ApplyEffect / ClearEffect against App().Effects.
using Effect = CS2Kit::EffectDescriptor;
using ParamEffect = CS2Kit::ParamEffectDescriptor;
using EffectInstance = CS2Kit::EffectInstance;
using EffectChoice = CS2Kit::EffectChoice;
using EffectScope = CS2Kit::EffectScope;

/** Disco: cycles bright render colors on a timer, auto-cancelling after a fixed duration. */
extern const Effect Disco;

/** Ghost: full invisibility via transmit filtering (pawn + weapons + wearables). */
extern const Effect Ghost;

/**
 * Hide: stealth-spectator. Moves the player to the spectator team in free-roam observer mode,
 * clears their scoreboard name, and grants glow vision (all live players outlined through walls,
 * visible only to them) for covert cheater observation; the toggle is silent - no broadcast.
 * Toggling off restores the original team and name. Self-only in practice - invoked via
 * ToggleEffect(App().Effects, adminSlot, adminSlot, Hide).
 */
extern const Effect Hide;

/**
 * Wallhack: the target sees every other live player as a team-colored glow through walls;
 * per-viewer transmit filtering keeps the glow entities invisible to everyone else.
 */
extern const Effect Wallhack;

/** Model: parameterized fun-model swap; the param indexes FunModels() (see Model.hpp). */
extern const ParamEffect Model;

}  // namespace AdminSystem::Admin::Effects
