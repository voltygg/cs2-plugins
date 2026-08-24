#pragma once

#include "../Actions/ActionContext.hpp"
#include "EffectId.hpp"

#include <VoltMod/Api.hpp>

namespace AdminSystem::Admin::Effects
{

// The effect scaffold (descriptors + policy-checked dispatch) lives in the kit; these aliases
// keep descriptor files and call sites on the established local names. Dispatch with
// VoltMod::ToggleEffect / ApplyEffect / ClearEffect against the plugin App effect manager.
using Effect = VoltMod::EffectDescriptor;
using ParamEffect = VoltMod::ParamEffectDescriptor;
using EffectInstance = VoltMod::EffectInstance;
using EffectChoice = VoltMod::EffectChoice;
using EffectScope = VoltMod::EffectScope;

/** Disco: cycles bright render colors on a timer, auto-cancelling after a fixed duration. */
extern const Effect Disco;

/** Ghost: full invisibility via transmit filtering (pawn + weapons + wearables). */
extern const Effect Ghost;

/**
 * Hide: stealth-spectator. Moves the player to the spectator team in free-roam observer mode,
 * clears their scoreboard name, and grants glow vision (all live players outlined through walls,
 * visible only to them) for covert cheater observation; the toggle is silent - no broadcast.
 * Toggling off restores the original team and name. Self-only in practice - invoked via
 * ToggleEffect(app.Effects, adminSlot, adminSlot, Hide).
 */
extern const Effect Hide;

/**
 * Wallhack: the target sees every other live player as a team-colored glow through walls;
 * per-viewer transmit filtering keeps the glow entities invisible to everyone else.
 */
extern const Effect Wallhack;

/** Model: parameterized fun-model swap; the param indexes FunModels() (see Model.hpp). */
extern const ParamEffect Model;

/**
 * Bhop: session bunnyhop grant, delivered cross-plugin - Setup/OnStop drive the bhop
 * plugin's `bhop_player` server command. Requires the bhop plugin in "grants" mode;
 * without it the command is unknown and the toggle is inert. Declared
 * EffectScope::Session, so it survives the death sweep; revoked on disconnect like
 * every session effect.
 */
extern const Effect Bhop;

}  // namespace AdminSystem::Admin::Effects
