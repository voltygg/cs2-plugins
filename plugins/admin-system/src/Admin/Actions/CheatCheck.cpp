#include "../../Core/App.hpp"
#include "../CheatCheck/CheatCheckManager.hpp"
#include "Descriptors.hpp"

namespace AdminSystem::Admin::Actions
{

bool CallCheck(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef target)
{
    if (!app.Actions.Resolve(admin, target, Flag(Permission::Control)))
        return false;

    return app.CheatCheck.StartCheck(admin.Slot, target.Slot);
}

bool CancelCheck(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef target)
{
    if (!app.Actions.Resolve(admin, target, Flag(Permission::Control)))
        return false;

    return app.CheatCheck.Cancel(target.Slot);
}

}  // namespace AdminSystem::Admin::Actions
