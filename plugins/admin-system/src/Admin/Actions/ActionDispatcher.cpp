#include "Admin/Actions/ActionDispatcher.hpp"

#include <VoltMod/Entities/EntitySystem.hpp>
#include <VoltMod/Players/PlayerManager.hpp>

using VoltMod::PlayerRef;
using VoltMod::Result;

namespace AdminSystem::Admin::Actions
{

Result<ActionContext> ActionDispatcher::Resolve(PlayerRef caller, PlayerRef target, std::string_view permission) const
{
    // The refs go through untouched: Authorize is what rejects one whose slot has changed hands.
    auto authorized = _policy.Authorize(caller, target, permission);
    if (!authorized)
    {
        return std::unexpected(authorized.error());
    }

    return ActionContext{.Auth = *authorized};
}

void ActionDispatcher::Run(PlayerRef caller, PlayerRef target, const Action& action) const
{
    auto ctx = Resolve(caller, target, action.Permission);
    if (!ctx)
    {
        return;
    }
    if (action.RequireAlive && !ctx->Target().Pawn().IsAlive())
    {
        return;
    }
    if (auto key = action.Body(*ctx))
    {
        Broadcast(*ctx, *key);
    }
}

void ActionDispatcher::Run(PlayerRef caller, PlayerRef target, int param, const ParamAction& action) const
{
    auto ctx = Resolve(caller, target, action.Permission);
    if (!ctx)
    {
        return;
    }
    if (action.RequireAlive && !ctx->Target().Pawn().IsAlive())
    {
        return;
    }
    if (auto key = action.Body(*ctx, param))
    {
        Broadcast(*ctx, *key);
    }
}

void ActionDispatcher::Broadcast(const ActionContext& ctx, std::string_view translationKey) const
{
    if (OnBroadcast)
    {
        OnBroadcast(ctx.Auth, translationKey);
    }
}

}  // namespace AdminSystem::Admin::Actions
