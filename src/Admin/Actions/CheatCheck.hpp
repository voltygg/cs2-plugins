#pragma once

namespace AdminSystem::Admin::Actions
{

void CallCheck(int adminSlot, int targetSlot);

/** Returns false if the action was rejected (immunity/permission) or no check was active. */
bool CancelCheck(int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Actions
