#include "Descriptors.hpp"

#include <CS2Kit/Sdk/PawnOps.hpp>

namespace AdminSystem::Admin::Actions
{

namespace PawnOps = CS2Kit::Sdk::PawnOps;

namespace
{
constexpr float BuryDepth = 15.0f;
}  // namespace

const Action Noclip{Permission::Control, /*requireAlive*/ false, [](const ActionContext& ctx) -> OptKey {
                        return PawnOps::ToggleNoclip(ctx.TargetCtrl) ? "broadcast.noclipOn" : "broadcast.noclipOff";
                    }};

const Action Freeze{Permission::Control, /*requireAlive*/ false, [](const ActionContext& ctx) -> OptKey {
                        return PawnOps::ToggleFreeze(ctx.TargetCtrl) ? "broadcast.freezeOn" : "broadcast.freezeOff";
                    }};

const Action Bury{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      PawnOps::ShiftZ(ctx.TargetCtrl, -BuryDepth);
                      return "broadcast.buried";
                  }};

const Action Unbury{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                        PawnOps::ShiftZ(ctx.TargetCtrl, BuryDepth);
                        return "broadcast.unburied";
                    }};

}  // namespace AdminSystem::Admin::Actions
