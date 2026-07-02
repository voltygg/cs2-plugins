#include "Descriptors.hpp"

#include <CS2Kit/Sdk/PawnOps.hpp>
#include <mathlib/vector.h>

namespace AdminSystem::Admin::Actions
{

namespace PawnOps = CS2Kit::Sdk::PawnOps;

const Action Bring{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                       if (!ctx.AdminCtrl.IsValid())
                           return std::nullopt;
                       Vector dest = PawnOps::ClearedDestination(ctx.AdminCtrl);
                       Vector zero{0.0f, 0.0f, 0.0f};
                       ctx.TargetCtrl.Teleport(&dest, nullptr, &zero);
                       return "broadcast.brought";
                   }};

const Action Goto{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      if (!ctx.AdminCtrl.IsValid())
                          return std::nullopt;
                      Vector dest = PawnOps::ClearedDestination(ctx.TargetCtrl);
                      Vector zero{0.0f, 0.0f, 0.0f};
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

    PawnOps::SwapOrigins(ctxA.TargetCtrl, ctxB.TargetCtrl);
    Broadcast(ctxA, ctxB, "broadcast.swapped");
}

}  // namespace AdminSystem::Admin::Actions
