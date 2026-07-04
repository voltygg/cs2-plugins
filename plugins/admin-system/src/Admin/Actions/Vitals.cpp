#include "Descriptors.hpp"

#include <CS2Kit/Sdk/PawnOps.hpp>

namespace AdminSystem::Admin::Actions
{

const Action Kill{Flag(Permission::Control), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      ctx.TargetCtrl.Slay();
                      return "broadcast.killed";
                  }};

const Action Godmode{Flag(Permission::Health), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                         return CS2Kit::Sdk::PawnOps::ToggleGodmode(ctx.TargetCtrl) ? "broadcast.godmodeOn"
                                                                                    : "broadcast.godmodeOff";
                     }};

const ParamAction SetHealth{Flag(Permission::Health), /*requireAlive*/ true,
                            [](const ActionContext& ctx, int health) -> OptKey {
                                ctx.TargetCtrl.SetHealth(health);
                                return "broadcast.healed";
                            }};

const ParamAction SetArmor{Flag(Permission::Health), /*requireAlive*/ true,
                           [](const ActionContext& ctx, int armor) -> OptKey {
                               ctx.TargetCtrl.SetArmor(armor);
                               return "broadcast.armored";
                           }};

const ParamAction SetSize{Flag(Permission::Fun), /*requireAlive*/ true,
                          [](const ActionContext& ctx, int percent) -> OptKey {
                              ctx.TargetCtrl.SetModelScale(percent / 100.0f);
                              return "broadcast.sizeSet";
                          }};

}  // namespace AdminSystem::Admin::Actions
