#pragma once

#include "Admin/Actions/ActionContext.hpp"
#include "Admin/Actions/PawnTimers.hpp"
#include "Core/Types.hpp"

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

/** Exchange origins between two already-resolved targets. */
void Swap(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef first, VoltMod::PlayerRef second);

/** Param is the destination VoltMod::Team; teams a player cannot join are ignored. */
extern const ParamAction ChangeTeam;

/** Param is the model-size percent (100 = normal); the framework clamps the scale. */
extern const ParamAction SetSize;

// These actions need services outside ActionContext, so App builds them once.

/** Upward punt with three seconds of fall protection. */
Action MakeSlap(PawnTimers& timers);

/** A no-damage explosion, then a slay 300 ms later. */
Action MakeSmite(VoltMod::Runtime& runtime, PawnTimers& timers);

/** Runtime-bound action descriptors owned by `App` for one load cycle. */
struct ActionDescriptors
{
    explicit ActionDescriptors(VoltMod::Runtime& runtime)
        : Timers(runtime), Slap(MakeSlap(Timers)), Smite(MakeSmite(runtime, Timers))
    {}

    /** Declared first: the actions below hold it. */
    PawnTimers Timers;
    Action Slap;
    Action Smite;
};

/** Returns false if the action was rejected (immunity/permission) or the check could not start. */
bool CallCheck(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef target);

/** Returns false if the action was rejected (immunity/permission) or no check was active. */
bool CancelCheck(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef target);

}  // namespace AdminSystem::Admin::Actions
