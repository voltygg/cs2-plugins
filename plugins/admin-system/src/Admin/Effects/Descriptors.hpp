#pragma once

#include "../Actions/ActionContext.hpp"
#include "EffectId.hpp"

#include <VoltMod/Api.hpp>

namespace AdminSystem::Admin::Effects
{

using Effect = VoltMod::EffectDescriptor;
using ParamEffect = VoltMod::ParamEffectDescriptor;
using EffectInstance = VoltMod::EffectInstance;
using EffectChoice = VoltMod::EffectChoice;
using EffectScope = VoltMod::EffectScope;

/** Cycle render colors until the effect expires. */
extern const Effect Disco;

/** Hides the pawn, weapons, and wearables through transmit filtering. */
extern const Effect Ghost;

/**
 * Move an admin to free-roam spectator with a hidden scoreboard name and private
 * glow vision. Restore the team and name on stop. This self-only effect is silent.
 */
extern const Effect Hide;

/**
 * Show live players as team-colored glows visible only to the target.
 */
extern const Effect Wallhack;

/** Model swap whose parameter indexes FunModels(). */
extern const ParamEffect Model;

/**
 * Grant session bunnyhop through `bhop_player`. Requires bhop `grants` mode,
 * survives death, and ends on disconnect.
 */
extern const Effect Bhop;

}  // namespace AdminSystem::Admin::Effects
