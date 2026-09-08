#pragma once

#include "../Actions/ActionContext.hpp"
#include "EffectId.hpp"

#include <VoltMod/Api.hpp>
#include <array>

namespace AdminSystem::Admin::Effects
{

using Effect = VoltMod::EffectDescriptor;
using EffectInstance = VoltMod::EffectInstance;
using EffectChoice = VoltMod::EffectChoice;
using EffectScope = VoltMod::EffectScope;

// Effect bodies need engine services outside ActionContext, so each descriptor captures Runtime
// while App builds the set for a load cycle.

/** Cycle render colors until the effect expires. */
Effect MakeDisco(VoltMod::Runtime& runtime);

/** Hides the pawn, weapons, and wearables through visibility filtering. */
Effect MakeGhost(VoltMod::Runtime& runtime);

/**
 * Move an admin to free-roam spectator with a hidden scoreboard name and private
 * glow vision. Restore the team and name on stop. This self-only effect is silent.
 */
Effect MakeHide(VoltMod::Runtime& runtime);

/** Show live players as team-colored glows visible only to the target. */
Effect MakeWallhack(VoltMod::Runtime& runtime);

/** Model swap picker whose choices index FunModels(). */
Effect MakeModel(VoltMod::Runtime& runtime);

/**
 * Grant session bunnyhop through `bhop_player`. Requires bhop `grants` mode,
 * survives death, and ends on disconnect.
 */
Effect MakeBhop(VoltMod::Runtime& runtime);

Effect MakeDrunk(VoltMod::Runtime& runtime);

/**
 * @brief Effect descriptors for one load cycle, owned by `App`.
 */
struct EffectDescriptors
{
    explicit EffectDescriptors(VoltMod::Runtime& runtime)
        : Disco(MakeDisco(runtime)),
          Ghost(MakeGhost(runtime)),
          Hide(MakeHide(runtime)),
          Wallhack(MakeWallhack(runtime)),
          Model(MakeModel(runtime)),
          Bhop(MakeBhop(runtime)),
          Drunk(MakeDrunk(runtime))
    {}

    Effect Disco;
    Effect Ghost;
    Effect Hide;
    Effect Wallhack;
    Effect Model;
    Effect Bhop;
    Effect Drunk;

    /** Menu order for auto-listed effects. Pointers remain valid for this object's lifetime.
     * Hide is a self-only Control row and `!hide` command. */
    const std::array<const Effect*, 6> MenuEffects{&Ghost, &Disco, &Wallhack, &Model, &Bhop, &Drunk};
};

}  // namespace AdminSystem::Admin::Effects
