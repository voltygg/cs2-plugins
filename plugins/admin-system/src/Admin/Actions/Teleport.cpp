#include "../../Core/App.hpp"
#include "Descriptors.hpp"

#include <VoltMod/Sdk/Entity/PawnOps.hpp>
#include <mathlib/vector.h>

namespace AdminSystem::Admin::Actions
{

namespace PawnOps = VoltMod::Sdk::PawnOps;

const Action Bring{Flag(Permission::Control), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                       if (!ctx.CallerCtrl.IsValid())
                           return std::nullopt;
                       Vector dest = PawnOps::ClearedDestination(ctx.CallerCtrl);
                       Vector zero{0.0f, 0.0f, 0.0f};
                       ctx.TargetCtrl.Teleport(&dest, nullptr, &zero);
                       return "broadcast.brought";
                   }};

const Action Goto{Flag(Permission::Control), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      if (!ctx.CallerCtrl.IsValid())
                          return std::nullopt;
                      Vector dest = PawnOps::ClearedDestination(ctx.TargetCtrl);
                      Vector zero{0.0f, 0.0f, 0.0f};
                      ctx.CallerCtrl.Teleport(&dest, nullptr, &zero);
                      return "broadcast.goto";
                  }};

void Swap(App& app, int adminSlot, int firstSlot, int secondSlot)
{
    if (firstSlot == secondSlot)
        return;
    auto ctxA = app.Actions.Resolve(adminSlot, firstSlot, Flag(Permission::Control));
    auto ctxB = app.Actions.Resolve(adminSlot, secondSlot, Flag(Permission::Control));
    if (!ctxA.Valid() || !ctxB.Valid())
        return;
    if (!ctxA.TargetCtrl.IsAlive() || !ctxB.TargetCtrl.IsAlive())
        return;

    PawnOps::SwapOrigins(ctxA.TargetCtrl, ctxB.TargetCtrl);
    Broadcast(app, ctxA, ctxB, "broadcast.swapped");
}

}  // namespace AdminSystem::Admin::Actions
