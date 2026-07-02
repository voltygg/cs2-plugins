#include "ActionContext.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Managers.hpp"

namespace AdminSystem::Admin::Actions
{

ActionContext Resolve(int adminSlot, int targetSlot, const std::string& requiredFlag)
{
    return App().ActionDisp.Resolve(adminSlot, targetSlot, requiredFlag);
}

void Broadcast(const ActionContext& ctx, const std::string& translationKey)
{
    App().ActionDisp.Broadcast(ctx, translationKey);
}

void Broadcast(const ActionContext& first, const ActionContext& second, const std::string& translationKey)
{
    if (!first.Caller || !first.Target || !second.Target)
        return;
    App().Chat.BroadcastAction(translationKey, first.Caller->GetName(),
                               {{"a", first.Target->GetName()}, {"b", second.Target->GetName()}});
}

void Run(int adminSlot, int targetSlot, const Action& action)
{
    App().ActionDisp.Run(adminSlot, targetSlot, action);
}

void Run(int adminSlot, int targetSlot, int param, const ParamAction& action)
{
    App().ActionDisp.Run(adminSlot, targetSlot, param, action);
}

}  // namespace AdminSystem::Admin::Actions
