#include "../../Core/App.hpp"
#include "../CheatCheck/CheatCheckManager.hpp"
#include "Descriptors.hpp"

namespace AdminSystem::Admin::Actions
{

bool CallCheck(App& app, int adminSlot, int targetSlot)
{
    if (!app.Actions.Resolve(adminSlot, targetSlot, Flag(Permission::Control)))
        return false;

    return app.CheatCheck.StartCheck(adminSlot, targetSlot);
}

bool CancelCheck(App& app, int adminSlot, int targetSlot)
{
    if (!app.Actions.Resolve(adminSlot, targetSlot, Flag(Permission::Control)))
        return false;

    return app.CheatCheck.Cancel(targetSlot);
}

}  // namespace AdminSystem::Admin::Actions
