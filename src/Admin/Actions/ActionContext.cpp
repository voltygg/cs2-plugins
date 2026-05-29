#include "ActionContext.hpp"
#include "../../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../../Core/ChatService.hpp"
#include "../AdminManager.hpp"

#include <CS2Kit/Players/PlayerManager.hpp>

using CS2Kit::Core::Kit;

namespace AdminSystem::Admin::Actions
{

using AdminSystem::Core::ChatService;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Sdk::PlayerController;

ActionContext Resolve(int adminSlot, int targetSlot, char requiredFlag)
{
    ActionContext ctx{nullptr, nullptr, PlayerController(adminSlot), PlayerController(targetSlot)};

    auto& plrMgr = Kit().Players;
    ctx.Admin = plrMgr.GetPlayerBySlot(adminSlot);
    ctx.Target = plrMgr.GetPlayerBySlot(targetSlot);

    if (!ctx.Admin || !ctx.Target)
        return ctx;

    auto& adminMgr = Sys().Admins;
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
    Sys().Chat.BroadcastAction(translationKey, ctx.Admin->GetName(), ctx.Target->GetName());
}

void RunAction(int adminSlot, int targetSlot, char requiredFlag,
               const std::function<std::optional<std::string>(const ActionContext&)>& body)
{
    auto ctx = Resolve(adminSlot, targetSlot, requiredFlag);
    if (!ctx.Valid())
        return;
    if (auto key = body(ctx))
        Broadcast(ctx, *key);
}

}  // namespace AdminSystem::Admin::Actions
