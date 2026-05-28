#include "CheatCheck.hpp"

#include "../CheatCheck/CheatCheckManager.hpp"
#include "ActionContext.hpp"

namespace AdminSystem::Admin::Actions
{

void DoCallCheck(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 'k');
    if (!ctx.Valid())
        return;

    CheatCheck::CheatCheckManager::Instance().StartCheck(adminSlot, targetSlot);
}

void DoCancelCheck(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 'k');
    if (!ctx.Valid())
        return;

    CheatCheck::CheatCheckManager::Instance().Cancel(targetSlot);
}

}  // namespace AdminSystem::Admin::Actions
