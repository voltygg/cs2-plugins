#include "ActionContext.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Managers.hpp"
#include "../AdminManager.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Actions
{

using AdminSystem::Core::ChatService;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Sdk::PlayerController;

ActionContext Resolve(int adminSlot, int targetSlot, char requiredFlag)
{
    ActionContext ctx{nullptr, nullptr, PlayerController(adminSlot), PlayerController(targetSlot)};

    auto& plrMgr = Engine().Players;
    ctx.Admin = plrMgr.GetPlayerBySlot(adminSlot);
    ctx.Target = plrMgr.GetPlayerBySlot(targetSlot);

    if (!ctx.Admin || !ctx.Target)
        return ctx;

    auto& adminMgr = App().Admins;
    int64_t adminSid = ctx.Admin->GetSteamID();
    int64_t targetSid = ctx.Target->GetSteamID();

    if (requiredFlag != '\0' && !adminMgr.HasPermission(adminSid, requiredFlag))
    {
        ctx.Admin = nullptr;
        return ctx;
    }
    if (!adminMgr.CanTarget(adminSid, targetSid))
    {
        ctx.Admin = nullptr;
        return ctx;
    }
    return ctx;
}

void Broadcast(const ActionContext& ctx, const std::string& translationKey)
{
    if (!ctx.Admin || !ctx.Target)
        return;
    App().Chat.BroadcastAction(translationKey, ctx.Admin->GetName(), ctx.Target->GetName());
}

void Run(int adminSlot, int targetSlot, const Action& action)
{
    auto ctx = Resolve(adminSlot, targetSlot, static_cast<char>(action.Flag));
    if (!ctx.Valid())
        return;
    if (action.RequireAlive && !ctx.TargetCtrl.IsAlive())
        return;
    if (auto key = action.Body(ctx))
        Broadcast(ctx, *key);
}

void Run(int adminSlot, int targetSlot, int param, const ParamAction& action)
{
    auto ctx = Resolve(adminSlot, targetSlot, static_cast<char>(action.Flag));
    if (!ctx.Valid())
        return;
    if (action.RequireAlive && !ctx.TargetCtrl.IsAlive())
        return;
    if (auto key = action.Body(ctx, param))
        Broadcast(ctx, *key);
}

}  // namespace AdminSystem::Admin::Actions
