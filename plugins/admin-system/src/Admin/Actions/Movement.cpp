#include "Descriptors.hpp"

#include <VoltMod/Entities/PawnOps.hpp>

namespace AdminSystem::Admin::Actions
{

namespace PawnOps = VoltMod::PawnOps;

static constexpr float BuryDepth = 15.0f;

const Action Noclip{Permission::Control, /*requireAlive*/ false, [](const ActionContext& ctx) -> OptKey {
                        return PawnOps::ToggleNoclip(ctx.TargetPawn()) ? "broadcast.noclipOn" : "broadcast.noclipOff";
                    }};

const Action Freeze{Permission::Control, /*requireAlive*/ false, [](const ActionContext& ctx) -> OptKey {
                        return PawnOps::ToggleFreeze(ctx.TargetPawn()) ? "broadcast.freezeOn" : "broadcast.freezeOff";
                    }};

const Action Bury{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      PawnOps::ShiftZ(ctx.TargetPawn(), -BuryDepth);
                      return "broadcast.buried";
                  }};

const Action Unbury{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                        PawnOps::ShiftZ(ctx.TargetPawn(), BuryDepth);
                        return "broadcast.unburied";
                    }};

const ParamAction SetSpeed{Permission::Control, /*requireAlive*/ true,
                           [](const ActionContext& ctx, int percent) -> OptKey {
                               ctx.TargetPawn().SetSpeedModifier(percent / 100.0f);
                               return "broadcast.speedSet";
                           }};

}  // namespace AdminSystem::Admin::Actions
