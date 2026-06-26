#include "../../Core/Managers.hpp"
#include "../CheatCheck/CheatCheckManager.hpp"
#include "Descriptors.hpp"

namespace AdminSystem::Admin::Actions
{

void CallCheck(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, static_cast<char>(Permission::CheatCheck));
    if (!ctx.Valid())
        return;

    App().CheatCheck.StartCheck(adminSlot, targetSlot);
}

bool CancelCheck(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, static_cast<char>(Permission::CheatCheck));
    if (!ctx.Valid())
        return false;

    return App().CheatCheck.Cancel(targetSlot);
}

}  // namespace AdminSystem::Admin::Actions
