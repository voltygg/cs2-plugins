#include "Descriptors.hpp"
#include "../../Core/Managers.hpp"

#include "../CheatCheck/CheatCheckManager.hpp"

namespace AdminSystem::Admin::Actions
{

void CallCheck(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, static_cast<char>(Permission::CheatCheck));
    if (!ctx.Valid())
        return;

    Sys().CheatCheck.StartCheck(adminSlot, targetSlot);
}

bool CancelCheck(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, static_cast<char>(Permission::CheatCheck));
    if (!ctx.Valid())
        return false;

    return Sys().CheatCheck.Cancel(targetSlot);
}

}  // namespace AdminSystem::Admin::Actions
