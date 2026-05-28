#pragma once

namespace AdminSystem::Admin::Actions
{

void DoCallCheck(int adminSlot, int targetSlot);

/** Returns false if the action was rejected (immunity/permission) or no check was active. */
bool DoCancelCheck(int adminSlot, int targetSlot);

}  // namespace AdminSystem::Admin::Actions
