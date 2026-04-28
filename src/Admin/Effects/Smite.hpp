#pragma once

namespace AdminSystem::Admin::Effects
{

/**
 * Theatrical instakill: 250 ms delay, then Slay. Per Phase 0 fallback for the missing
 * particle API — no lightning bolt visual until ParticleService lands. The chat broadcast
 * carries the drama in the meantime.
 */
void DoSmite(int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Effects
