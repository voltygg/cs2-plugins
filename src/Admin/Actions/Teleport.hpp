#pragma once

namespace AdminSystem::Admin::Actions
{

void DoBring(int adminSlot, int targetSlot);
void DoGoto(int adminSlot, int targetSlot);

/** Exchange origins between two targets. The picker for the second target is built
 *  by the menu layer; this entry point assumes both are already resolved. */
void DoSwap(int adminSlot, int firstSlot, int secondSlot);

}  // namespace AdminSystem::Admin::Actions
