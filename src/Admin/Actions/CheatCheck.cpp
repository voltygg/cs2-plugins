#include "CheatCheck.hpp"
#include "../../Core/Managers.hpp"

#include "../CheatCheck/CheatCheckManager.hpp"
#include "ActionContext.hpp"

namespace AdminSystem::Admin::Actions
{

void DoCallCheck(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 'k');
    if (!ctx.Valid())
        return;

    Sys().CheatCheck.StartCheck(adminSlot, targetSlot);
}

bool DoCancelCheck(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 'k');
    if (!ctx.Valid())
        return false;

    return Sys().CheatCheck.Cancel(targetSlot);
}

}  // namespace AdminSystem::Admin::Actions
