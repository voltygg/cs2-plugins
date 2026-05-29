#pragma once

#include "ActionContext.hpp"

namespace AdminSystem::Admin::Actions
{

extern const Action Bring;
extern const Action Goto;

/** Exchange origins between two targets. The picker for the second target is built
 *  by the menu layer; this entry point assumes both are already resolved. */
void Swap(int adminSlot, int firstSlot, int secondSlot);

}  // namespace AdminSystem::Admin::Actions
