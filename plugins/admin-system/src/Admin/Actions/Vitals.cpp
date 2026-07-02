#include "Descriptors.hpp"

#include <CS2Kit/Sdk/PawnOps.hpp>

namespace AdminSystem::Admin::Actions
{

const Action Kill{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      ctx.TargetCtrl.Slay();
                      return "broadcast.killed";
                  }};

const Action Godmode{Permission::Health, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                         return CS2Kit::Sdk::PawnOps::ToggleGodmode(ctx.TargetCtrl) ? "broadcast.godmodeOn"
                                                                                    : "broadcast.godmodeOff";
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
