#include "Teleport.hpp"

#include "ActionContext.hpp"

#include <mathlib/vector.h>

namespace AdminSystem::Admin::Actions
{

void DoBring(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid() || !ctx.AdminCtrl.IsValid() || !ctx.TargetCtrl.IsAlive())
        return;
    Vector dest = ctx.AdminCtrl.GetAbsOrigin();
    Vector zeroVel{0.0f, 0.0f, 0.0f};
    ctx.TargetCtrl.Teleport(&dest, nullptr, &zeroVel);
    Broadcast(ctx, "broadcast.brought");
}

void DoGoto(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid() || !ctx.AdminCtrl.IsValid() || !ctx.TargetCtrl.IsAlive())
        return;
    Vector dest = ctx.TargetCtrl.GetAbsOrigin();
    Vector zeroVel{0.0f, 0.0f, 0.0f};
    ctx.AdminCtrl.Teleport(&dest, nullptr, &zeroVel);
    Broadcast(ctx, "broadcast.goto");
}

void DoSwap(int adminSlot, int firstSlot, int secondSlot)
{
    if (firstSlot == secondSlot)
        return;
    auto ctxA = Resolve(adminSlot, firstSlot, 's');
    auto ctxB = Resolve(adminSlot, secondSlot, 's');
    if (!ctxA.Valid() || !ctxB.Valid())
        return;
    if (!ctxA.TargetCtrl.IsAlive() || !ctxB.TargetCtrl.IsAlive())
        return;

    Vector posA = ctxA.TargetCtrl.GetAbsOrigin();
    Vector posB = ctxB.TargetCtrl.GetAbsOrigin();
    Vector zeroVel{0.0f, 0.0f, 0.0f};

    ctxA.TargetCtrl.Teleport(&posB, nullptr, &zeroVel);
    ctxB.TargetCtrl.Teleport(&posA, nullptr, &zeroVel);

    // Two broadcasts so each player's name appears as the actor in the swap line.
    Broadcast(ctxA, "broadcast.swapped");
}

}  // namespace AdminSystem::Admin::Actions
