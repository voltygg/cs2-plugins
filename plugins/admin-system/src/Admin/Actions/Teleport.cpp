#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "Descriptors.hpp"

#include <VoltMod/Entities/PawnOps.hpp>
#include <mathlib/vector.h>

namespace AdminSystem::Admin::Actions
{

namespace PawnOps = VoltMod::Entities::PawnOps;

namespace
{
/** Two-target broadcast: the phrase receives the target names as {a} and {b}. Swap is the only
 *  action that resolves a pair, so this stays local to it. */
void BroadcastPair(App& app, const ActionContext& first, const ActionContext& second, const std::string& key)
{
    if (!first.Caller || !first.Target || !second.Target)
        return;
    app.Chat.BroadcastAction(key, first.Caller->GetName(),
                             {{"a", first.Target->GetName()}, {"b", second.Target->GetName()}});
}
}  // namespace

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
    BroadcastPair(app, ctxA, ctxB, "broadcast.swapped");
}

}  // namespace AdminSystem::Admin::Actions
