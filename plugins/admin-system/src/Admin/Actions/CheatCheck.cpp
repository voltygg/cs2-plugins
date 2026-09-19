#include "Admin/Actions/Descriptors.hpp"
#include "Admin/CheatCheck/CheatCheckManager.hpp"
#include "Core/App.hpp"

namespace AdminSystem::Admin::Actions
{

bool CallCheck(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef target)
{
    if (!app.Actions.Resolve(admin, target, Permission::Control))
        return false;

    return app.CheatCheck.StartCheck(admin.Slot, target.Slot);
}

bool CancelCheck(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef target)
{
    if (!app.Actions.Resolve(admin, target, Permission::Control))
        return false;

    return app.CheatCheck.Cancel(admin.Slot, target.Slot);
}

}  // namespace AdminSystem::Admin::Actions
