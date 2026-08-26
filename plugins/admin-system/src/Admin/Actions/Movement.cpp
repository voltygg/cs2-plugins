#include "Descriptors.hpp"

#include <VoltMod/Entities/PawnOps.hpp>

namespace AdminSystem::Admin::Actions
{

namespace PawnOps = VoltMod::Entities::PawnOps;

namespace
{
constexpr float BuryDepth = 15.0f;
}  // namespace

const Action Noclip{Flag(Permission::Control), /*requireAlive*/ false, [](const ActionContext& ctx) -> OptKey {
                        return PawnOps::ToggleNoclip(ctx.TargetCtrl) ? "broadcast.noclipOn" : "broadcast.noclipOff";
                    }};

const Action Freeze{Flag(Permission::Control), /*requireAlive*/ false, [](const ActionContext& ctx) -> OptKey {
                        return PawnOps::ToggleFreeze(ctx.TargetCtrl) ? "broadcast.freezeOn" : "broadcast.freezeOff";
                    }};

const Action Bury{Flag(Permission::Control), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      PawnOps::ShiftZ(ctx.TargetCtrl, -BuryDepth);
                      return "broadcast.buried";
                  }};

const Action Unbury{Flag(Permission::Control), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                        PawnOps::ShiftZ(ctx.TargetCtrl, BuryDepth);
                        return "broadcast.unburied";
                    }};

const ParamAction SetSpeed{Flag(Permission::Control), /*requireAlive*/ true,
                           [](const ActionContext& ctx, int percent) -> OptKey {
                               ctx.TargetCtrl.SetSpeedModifier(percent / 100.0f);
                               return "broadcast.speedSet";
                           }};

}  // namespace AdminSystem::Admin::Actions
