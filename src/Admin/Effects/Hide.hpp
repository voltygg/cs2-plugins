#pragma once

namespace AdminSystem::Admin::Effects
{

/**
 * Toggle stealth-spectator on the calling admin: moves them to the spectator
 * team in free-roam observer mode and clears their scoreboard name. Re-applying
 * restores the original team and name.
 *
 * Self-only — requires permission flag 'b'.
 */
void ToggleHide(int adminSlot);

}  // namespace AdminSystem::Admin::Effects
