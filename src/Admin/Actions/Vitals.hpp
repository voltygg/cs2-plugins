#pragma once

namespace AdminSystem::Admin::Actions
{

void DoSlay(int adminSlot, int targetSlot);
void DoSetHealth(int adminSlot, int targetSlot, int health);
void DoSetArmor(int adminSlot, int targetSlot, int armor);
void DoToggleGodmode(int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Actions
