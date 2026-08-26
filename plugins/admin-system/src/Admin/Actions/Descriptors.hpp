#pragma once

#include "../../Core/App.hpp"
#include "ActionContext.hpp"

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

/** Param is the model-size percent (100 = normal); the body divides by 100 and the framework clamps it. */
extern const ParamAction SetSize;

extern const Action Bring;
extern const Action Goto;

/** Exchange origins between two targets. The picker for the second target is built
 *  by the menu layer; this entry point assumes both are already resolved. */
void Swap(App& app, int adminSlot, int firstSlot, int secondSlot);

/** Param is the destination team (VoltMod::TeamSpectator/TeamT/TeamCT); out-of-range values are ignored. */
extern const ParamAction ChangeTeam;

/** Apply upward velocity and three seconds of fall protection. */
extern const Action Slap;

/**
 * Spawn a no-damage explosion, then slay after 300 ms. Fall back to delayed slay
 * when entity operations are unavailable.
 */
extern const Action Smite;

/** Returns false if the action was rejected (immunity/permission) or the check could not start. */
bool CallCheck(App& app, int adminSlot, int targetSlot);

/** Returns false if the action was rejected (immunity/permission) or no check was active. */
bool CancelCheck(App& app, int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Actions
