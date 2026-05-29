#include "Teleport.hpp"

#include <mathlib/vector.h>

namespace AdminSystem::Admin::Actions
{

namespace
{
const Vector ZeroVelocity{0.0f, 0.0f, 0.0f};
}  // namespace

const Action Bring{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                       if (!ctx.AdminCtrl.IsValid())
                           return std::nullopt;
                       Vector dest = ctx.AdminCtrl.GetAbsOrigin();
                       Vector zero = ZeroVelocity;
                       ctx.TargetCtrl.Teleport(&dest, nullptr, &zero);
                       return "broadcast.brought";
                   }};

const Action Goto{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      if (!ctx.AdminCtrl.IsValid())
                          return std::nullopt;
                      Vector dest = ctx.TargetCtrl.GetAbsOrigin();
                      Vector zero = ZeroVelocity;
                      ctx.AdminCtrl.Teleport(&dest, nullptr, &zero);
                      return "broadcast.goto";
                  }};

void Swap(int adminSlot, int firstSlot, int secondSlot)
{
    if (firstSlot == secondSlot)
        return;
    auto ctxA = Resolve(adminSlot, firstSlot, static_cast<char>(Permission::Control));
    auto ctxB = Resolve(adminSlot, secondSlot, static_cast<char>(Permission::Control));
    if (!ctxA.Valid() || !ctxB.Valid())
        return;
    if (!ctxA.TargetCtrl.IsAlive() || !ctxB.TargetCtrl.IsAlive())
        return;

    Vector posA = ctxA.TargetCtrl.GetAbsOrigin();
    Vector posB = ctxB.TargetCtrl.GetAbsOrigin();
    Vector zero = ZeroVelocity;

    ctxA.TargetCtrl.Teleport(&posB, nullptr, &zero);
    ctxB.TargetCtrl.Teleport(&posA, nullptr, &zero);

    Broadcast(ctxA, "broadcast.swapped");
}

}  // namespace AdminSystem::Admin::Actions
