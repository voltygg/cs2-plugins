#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "Descriptors.hpp"

#include <VoltMod/Entities/PawnOps.hpp>
#include <mathlib/vector.h>
#include <optional>

namespace AdminSystem::Admin::Actions
{

namespace PawnOps = VoltMod::PawnOps;

/** Two-target broadcast: the phrase receives the target names as {a} and {b}. Swap is the only
 *  action that resolves a pair, so this stays local to it. */
static void BroadcastPair(App& app, const ActionContext& first, const ActionContext& second, const std::string& key)
{
    if (!first.Caller || !first.Target || !second.Target)
        return;
    app.Chat.BroadcastAction(key, first.Caller->GetName(),
                             {{"a", first.Target->GetName()}, {"b", second.Target->GetName()}});
}

const Action Bring{Flag(Permission::Control), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                       if (!ctx.CallerPawn())
                           return std::nullopt;
                       Vector dest = PawnOps::ClearedDestination(ctx.CallerPawn());
                       (void)ctx.TargetPawn().Teleport(dest, std::nullopt, Vector{0.0f, 0.0f, 0.0f});
                       return "broadcast.brought";
                   }};

const Action Goto{Flag(Permission::Control), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      if (!ctx.CallerPawn())
                          return std::nullopt;
                      Vector dest = PawnOps::ClearedDestination(ctx.TargetPawn());
                      (void)ctx.CallerPawn().Teleport(dest, std::nullopt, Vector{0.0f, 0.0f, 0.0f});
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
    if (!ctxA.TargetPawn().IsAlive() || !ctxB.TargetPawn().IsAlive())
        return;

    PawnOps::SwapOrigins(ctxA.TargetPawn(), ctxB.TargetPawn());
    BroadcastPair(app, ctxA, ctxB, "broadcast.swapped");
}

}  // namespace AdminSystem::Admin::Actions
