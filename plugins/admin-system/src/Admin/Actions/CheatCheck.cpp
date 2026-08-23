#include "../../Core/App.hpp"
#include "../CheatCheck/CheatCheckManager.hpp"
#include "Descriptors.hpp"

namespace AdminSystem::Admin::Actions
{

bool CallCheck(App& app, int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, Flag(Permission::Control));
    if (!ctx.Valid())
        return false;

    return app.CheatCheck.StartCheck(adminSlot, targetSlot);
}

bool CancelCheck(App& app, int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, Flag(Permission::Control));
    if (!ctx.Valid())
        return false;

    return app.CheatCheck.Cancel(targetSlot);
}

}  // namespace AdminSystem::Admin::Actions
