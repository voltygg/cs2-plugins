#include "WeaponActions.hpp"

#include "../Admin/Actions/ActionContext.hpp"
#include "../Core/App.hpp"
#include "../Core/Permissions.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <functional>
#include <string>

namespace AdminSystem::Weapons
{

using Admin::Actions::Action;
using Admin::Actions::ActionContext;
using Admin::Actions::OptKey;

/**
 * Run @p body under the weapon permission and the shared broadcast policy.
 *
 * `RequireAlive` is left off and the aliveness check done here instead, because the dispatcher
 * skips a dead target silently and the chat commands need to say why nothing happened.
 */
static WeaponActionResult RunWeaponAction(App& app, int adminSlot, int targetSlot,
                                          const std::function<bool(const ActionContext&)>& body,
                                          const char* broadcastKey)
{
    auto outcome = WeaponActionResult::NotAllowed;

    app.Actions.Run(adminSlot, targetSlot,
                    Action{.Permission = Flag(Permission::Weapon),
                           .RequireAlive = false,
                           .Body = [&](const ActionContext& ctx) -> OptKey {
                               if (!ctx.TargetPawn().IsAlive())
                               {
                                   outcome = WeaponActionResult::TargetDead;
                                   return std::nullopt;
                               }
                               if (!body(ctx))
                               {
                                   outcome = WeaponActionResult::EngineRefused;
                                   return std::nullopt;
                               }
                               outcome = WeaponActionResult::Ok;
                               return broadcastKey;
                           }});

    return outcome;
}

WeaponActionResult GiveWeapon(App& app, int adminSlot, int targetSlot, std::string_view item)
{
    const std::string classname(item);
    return RunWeaponAction(
        app, adminSlot, targetSlot,
        [&classname](const ActionContext& ctx) { return ctx.Rt.Items.Give(ctx.TargetPawn(), classname.c_str()); },
        "broadcast.gaveWeapon");
}

WeaponActionResult StripWeapons(App& app, int adminSlot, int targetSlot)
{
    return RunWeaponAction(
        app, adminSlot, targetSlot,
        [](const ActionContext& ctx) { return ctx.Rt.Items.StripWeapons(ctx.TargetPawn()); }, "broadcast.stripped");
}

}  // namespace AdminSystem::Weapons
