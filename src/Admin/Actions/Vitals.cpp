#include "Vitals.hpp"

#include <CS2Kit/Sdk/Entity.hpp>

namespace AdminSystem::Admin::Actions
{

const Action Slay{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      ctx.TargetCtrl.Slay();
                      return "broadcast.slain";
                  }};

const Action Godmode{Permission::Health, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                         uint32_t flags = ctx.TargetCtrl.GetFlags();
                         bool turningOn = (flags & CS2Kit::Sdk::FL_GODMODE) == 0;
                         ctx.TargetCtrl.SetFlags(turningOn ? (flags | CS2Kit::Sdk::FL_GODMODE)
                                                           : (flags & ~CS2Kit::Sdk::FL_GODMODE));
                         return turningOn ? "broadcast.godmodeOn" : "broadcast.godmodeOff";
                     }};

const ParamAction SetHealth{Permission::Health, /*requireAlive*/ true,
                            [](const ActionContext& ctx, int health) -> OptKey {
                                ctx.TargetCtrl.SetHealth(health);
                                return "broadcast.healed";
                            }};

const ParamAction SetArmor{Permission::Health, /*requireAlive*/ true,
                           [](const ActionContext& ctx, int armor) -> OptKey {
                               ctx.TargetCtrl.SetArmor(armor);
                               return "broadcast.armored";
                           }};

}  // namespace AdminSystem::Admin::Actions
