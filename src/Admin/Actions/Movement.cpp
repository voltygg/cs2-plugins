#include "Movement.hpp"

#include <CS2Kit/Sdk/MoveType.hpp>
#include <mathlib/vector.h>

namespace AdminSystem::Admin::Actions
{

using CS2Kit::Sdk::MoveType;

namespace
{
constexpr float BuryDepth = 15.0f;

void ShiftZ(const CS2Kit::Sdk::PlayerController& pc, float deltaZ)
{
    Vector origin = pc.GetAbsOrigin();
    origin.z += deltaZ;
    pc.Teleport(&origin, nullptr, nullptr);
}
}  // namespace

const Action Noclip{Permission::Control, /*requireAlive*/ false, [](const ActionContext& ctx) -> OptKey {
                        bool turningOn = (ctx.TargetCtrl.GetMoveType() != MoveType::NoClip);
                        ctx.TargetCtrl.SetMoveType(turningOn ? MoveType::NoClip : MoveType::Walk);
                        return turningOn ? "broadcast.noclipOn" : "broadcast.noclipOff";
                    }};

const Action Freeze{Permission::Control, /*requireAlive*/ false, [](const ActionContext& ctx) -> OptKey {
                        bool turningOn = (ctx.TargetCtrl.GetMoveType() != MoveType::None);
                        ctx.TargetCtrl.SetMoveType(turningOn ? MoveType::None : MoveType::Walk);
                        return turningOn ? "broadcast.freezeOn" : "broadcast.freezeOff";
                    }};

const Action Bury{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      ShiftZ(ctx.TargetCtrl, -BuryDepth);
                      return "broadcast.buried";
                  }};

const Action Unbury{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                        ShiftZ(ctx.TargetCtrl, BuryDepth);
                        return "broadcast.unburied";
                    }};

}  // namespace AdminSystem::Admin::Actions
