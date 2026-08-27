#pragma once

#include "../../Core/Types.hpp"
#include "ActionContext.hpp"

#include <VoltMod/Api.hpp>

namespace AdminSystem::Admin::Actions
{

extern const Action Kill;
extern const Action Godmode;
extern const ParamAction SetHealth;
extern const ParamAction SetArmor;

extern const Action Noclip;
extern const Action Freeze;
extern const Action Bury;
extern const Action Unbury;
/** Param is the movement-speed percent (100 = normal); the body divides by 100. */
extern const ParamAction SetSpeed;

extern const Action Bring;
extern const Action Goto;

/** Exchange origins between two targets. The picker for the second target is built
 *  by the menu layer; this entry point assumes both are already resolved. */
void Swap(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef first, VoltMod::PlayerRef second);

/** Param is the destination team (VoltMod::TeamSpectator/TeamT/TeamCT); out-of-range values are ignored. */
extern const ParamAction ChangeTeam;

// Slap, Smite and SetSize reach an engine service (Pawns, EntityOps) an ActionContext does not
// carry, so - like the effect descriptors - they are built from a Runtime& by a factory function
// rather than declared as file-scope constants. AdminSystem::App owns the built instances next to
// Actions, its ActionDispatcher.

/** Apply upward velocity and three seconds of fall protection. */
Action MakeSlap(VoltMod::Runtime& runtime);

/**
 * Spawn a no-damage explosion, then slay after 300 ms. Fall back to delayed slay
 * when entity operations are unavailable.
 */
Action MakeSmite(VoltMod::Runtime& runtime);

/** Param is the model-size percent (100 = normal); the body divides by 100 and the framework clamps it. */
ParamAction MakeSetSize(VoltMod::Runtime& runtime);

/** Every Runtime&-bound action descriptor for one load cycle. Owned by `App` next to `Actions`. */
struct ActionDescriptors
{
    explicit ActionDescriptors(VoltMod::Runtime& runtime)
        : Slap(MakeSlap(runtime)), Smite(MakeSmite(runtime)), SetSize(MakeSetSize(runtime))
    {}

    Action Slap;
    Action Smite;
    ParamAction SetSize;
};

/** Returns false if the action was rejected (immunity/permission) or the check could not start. */
bool CallCheck(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef target);

/** Returns false if the action was rejected (immunity/permission) or no check was active. */
bool CancelCheck(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef target);

}  // namespace AdminSystem::Admin::Actions
