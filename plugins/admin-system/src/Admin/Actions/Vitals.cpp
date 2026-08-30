#include "Descriptors.hpp"

#include <VoltMod/Entities/PawnOps.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Actions
{

const Action Kill{Flag(Permission::Control), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      (void)ctx.TargetPawn().Slay();
                      return "broadcast.killed";
                  }};

const Action Godmode{Flag(Permission::Health), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                         return VoltMod::PawnOps::ToggleGodmode(ctx.TargetPawn()) ? "broadcast.godmodeOn"
                                                                                  : "broadcast.godmodeOff";
                     }};

const ParamAction SetHealth{Flag(Permission::Health), /*requireAlive*/ true,
                            [](const ActionContext& ctx, int health) -> OptKey {
                                ctx.TargetPawn().SetHealth(health);
                                return "broadcast.healed";
                            }};

const ParamAction SetArmor{Flag(Permission::Health), /*requireAlive*/ true,
                           [](const ActionContext& ctx, int armor) -> OptKey {
                               ctx.TargetPawn().SetArmor(armor);
                               return "broadcast.armored";
                           }};

ParamAction MakeSetSize(VoltMod::Runtime& runtime)
{
    return ParamAction{Flag(Permission::Fun), /*requireAlive*/ true,
                       [&runtime](const ActionContext& ctx, int percent) -> OptKey {
                           runtime.World.EntityOps.SetModelScale(ctx.TargetPawn().Raw(), percent / 100.0f);
                           return "broadcast.sizeSet";
                       }};
}

}  // namespace AdminSystem::Admin::Actions
