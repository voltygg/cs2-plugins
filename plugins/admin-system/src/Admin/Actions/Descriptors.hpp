#pragma once

#include "ActionContext.hpp"

namespace AdminSystem::Admin::Actions
{

// Vitals
extern const Action Kill;
extern const Action Godmode;
extern const ParamAction SetHealth;
extern const ParamAction SetArmor;

// Movement
extern const Action Noclip;
extern const Action Freeze;
extern const Action Bury;
extern const Action Unbury;

// Teleport
extern const Action Bring;
extern const Action Goto;

/** Exchange origins between two targets. The picker for the second target is built
 *  by the menu layer; this entry point assumes both are already resolved. */
void Swap(int adminSlot, int firstSlot, int secondSlot);

// Team
/** Param is the destination team (CS2Kit::Sdk::TeamSpectator/TeamT/TeamCT); out-of-range values are ignored. */
extern const ParamAction ChangeTeam;

// Slap
/**
 * @brief Slap: high upward velocity with a 3-second godmode window so the target
 * survives the fall.
 */
extern const Action Slap;

// Smite
/**
 * Theatrical instakill: a no-damage env_explosion at the target (stock fireball +
 * boom, works on every map without precaching), then Slay after a 300 ms beat.
 * Falls back to the plain delayed Slay when the kit's entity-ops signatures are
 * unresolved. Requires the Fun flag.
 */
extern const Action Smite;

// CheatCheck
void CallCheck(int adminSlot, int targetSlot);

/** Returns false if the action was rejected (immunity/permission) or no check was active. */
bool CancelCheck(int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Actions
