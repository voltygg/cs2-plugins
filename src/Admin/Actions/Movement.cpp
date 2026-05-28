#include "Movement.hpp"

#include "ActionContext.hpp"

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

void DoNoclip(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid())
        return;

    bool turningOn = (ctx.TargetCtrl.GetMoveType() != MoveType::NoClip);
    ctx.TargetCtrl.SetMoveType(turningOn ? MoveType::NoClip : MoveType::Walk);
    Broadcast(ctx, turningOn ? "broadcast.noclipOn" : "broadcast.noclipOff");
}

void DoFreeze(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid())
        return;

    bool turningOn = (ctx.TargetCtrl.GetMoveType() != MoveType::None);
    ctx.TargetCtrl.SetMoveType(turningOn ? MoveType::None : MoveType::Walk);
    Broadcast(ctx, turningOn ? "broadcast.freezeOn" : "broadcast.freezeOff");
}

void DoBury(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid() || !ctx.TargetCtrl.IsAlive())
        return;
    ShiftZ(ctx.TargetCtrl, -BuryDepth);
    Broadcast(ctx, "broadcast.buried");
}

void DoUnbury(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid() || !ctx.TargetCtrl.IsAlive())
        return;
    ShiftZ(ctx.TargetCtrl, BuryDepth);
    Broadcast(ctx, "broadcast.unburied");
}

}  // namespace AdminSystem::Admin::Actions
