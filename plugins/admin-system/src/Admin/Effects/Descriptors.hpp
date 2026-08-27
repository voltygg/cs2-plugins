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

// Each factory below builds one Effect. A body needs an engine service (Transmit, Entities,
// ConVars, ...) it cannot reach through ActionContext - an EffectDescriptor is data built once at
// load, before any per-request context exists - so it captures the Runtime& its factory is given.

/** Cycle render colors until the effect expires. */
Effect MakeDisco(VoltMod::Runtime& runtime);

/** Hides the pawn, weapons, and wearables through transmit filtering. */
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
 * @brief Every effect descriptor for one load cycle, built from the @ref VoltMod::Runtime the
 * bodies act through. Owned by `App`, next to the `EffectManager`/`EffectDispatcher` that drive
 * them - see `Core/App.hpp`.
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

    /** Every auto-listed effect, in the order the menu renders them. Points into the members
     *  above, so it is stable for this object's lifetime (one load cycle). Hide is missing on
     *  purpose: it is a self-only Control row and the `!hide` command. */
    const std::array<const Effect*, 6> MenuEffects{&Ghost, &Disco, &Wallhack, &Model, &Bhop, &Drunk};
};

}  // namespace AdminSystem::Admin::Effects
