#include "../../Core/Managers.hpp"
#include "../CheatCheck/CheatCheckManager.hpp"
#include "Descriptors.hpp"

namespace AdminSystem::Admin::Actions
{

bool CallCheck(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, Flag(Permission::Control));
    if (!ctx.Valid())
        return false;

    return App().CheatCheck.StartCheck(adminSlot, targetSlot);
}

bool CancelCheck(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, Flag(Permission::Control));
    if (!ctx.Valid())
        return false;

    return App().CheatCheck.Cancel(targetSlot);
}

}  // namespace AdminSystem::Admin::Actions
