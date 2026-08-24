#pragma once

#include "../Actions/ActionContext.hpp"
#include "EffectId.hpp"

#include <VoltMod/Api.hpp>

namespace AdminSystem::Admin::Effects
{

// Keep existing local names while using the framework's effect descriptors.
using Effect = VoltMod::EffectDescriptor;
using ParamEffect = VoltMod::ParamEffectDescriptor;
using EffectInstance = VoltMod::EffectInstance;
using EffectChoice = VoltMod::EffectChoice;
using EffectScope = VoltMod::EffectScope;

/** Cycles bright render colors until its timer expires. */
extern const Effect Disco;

/** Hides the pawn, weapons, and wearables through transmit filtering. */
extern const Effect Ghost;

/**
 * Moves an admin to free-roam spectator, hides their scoreboard name, and gives
 * them private glow vision. Disabling it restores the original team and name.
 * This self-only effect does not broadcast.
 */
extern const Effect Hide;

/**
 * Shows live players to the target as team-colored glows. Per-viewer transmit
 * filtering hides those glows from everyone else.
 */
extern const Effect Wallhack;

/** Model swap whose parameter indexes FunModels(). */
extern const ParamEffect Model;

/**
 * Session bunnyhop grant sent through the bhop plugin's `bhop_player` command.
 * It requires bhop in `grants` mode, survives death, and ends on disconnect.
 */
extern const Effect Bhop;

}  // namespace AdminSystem::Admin::Effects
