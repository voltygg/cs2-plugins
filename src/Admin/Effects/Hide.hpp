#pragma once

#include "EffectAction.hpp"

namespace AdminSystem::Admin::Effects
{

/**
 * Toggle stealth-spectator on a player: moves them to the spectator team in free-roam
 * observer mode and clears their scoreboard name. Re-applying restores the original team
 * and name. Self-only in practice — invoked via Run(adminSlot, adminSlot, Hide).
 */
extern const EffectToggle Hide;

}  // namespace AdminSystem::Admin::Effects
