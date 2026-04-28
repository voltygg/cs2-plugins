#pragma once

namespace AdminSystem::Admin::Effects
{

/** Toggle Ghost: translucent render + reduced alpha. Re-applying cancels. */
void ToggleGhost(int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Effects
