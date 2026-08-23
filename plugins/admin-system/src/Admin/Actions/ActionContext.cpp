#include "ActionContext.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Managers.hpp"

namespace AdminSystem::Admin::Actions
{

// The dispatcher is stateless - policy comes from Engine().Core.Policy, set once in OnLoad.
using CS2Kit::Players::ActionDispatcher;

ActionContext Resolve(int adminSlot, int targetSlot, const std::string& requiredFlag)
{
    return ActionDispatcher{}.Resolve(adminSlot, targetSlot, requiredFlag);
}

void Broadcast(const ActionContext& ctx, const std::string& translationKey)
{
    // An empty key means "no message" - silent effects (Hide) and one-shots leave it blank.
    if (translationKey.empty())
        return;
    ActionDispatcher{}.Broadcast(ctx, translationKey);
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
    ActionDispatcher{}.Run(adminSlot, targetSlot, action);
}

void Run(int adminSlot, int targetSlot, int param, const ParamAction& action)
{
    ActionDispatcher{}.Run(adminSlot, targetSlot, param, action);
}

}  // namespace AdminSystem::Admin::Actions
