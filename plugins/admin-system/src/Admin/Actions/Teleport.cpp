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
    app.Chat.BroadcastAction(key, first.Caller().Name(), {{"a", first.Target().Name()}, {"b", second.Target().Name()}});
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

void Swap(App& app, VoltMod::PlayerRef admin, VoltMod::PlayerRef first, VoltMod::PlayerRef second)
{
    if (first == second)
        return;
    auto ctxA = app.Actions.Resolve(admin, first, Flag(Permission::Control));
    auto ctxB = app.Actions.Resolve(admin, second, Flag(Permission::Control));
    if (!ctxA || !ctxB)
        return;
    if (!ctxA->TargetPawn().IsAlive() || !ctxB->TargetPawn().IsAlive())
        return;

    PawnOps::SwapOrigins(ctxA->TargetPawn(), ctxB->TargetPawn());
    BroadcastPair(app, *ctxA, *ctxB, "broadcast.swapped");
}

}  // namespace AdminSystem::Admin::Actions
