#include "ActionContext.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"

namespace AdminSystem::Admin::Actions
{

// The dispatcher is stateless - policy comes from app.Runtime.Policy, set once in OnLoad.
using VoltMod::Players::ActionDispatcher;

ActionContext Resolve(int adminSlot, int targetSlot, const std::string& requiredFlag)
{
    return ActionDispatcher{}.Resolve(adminSlot, targetSlot, requiredFlag);
}

void Broadcast(App& app, const ActionContext& first, const ActionContext& second, const std::string& translationKey)
{
    if (!first.Caller || !first.Target || !second.Target)
        return;
    app.Chat.BroadcastAction(translationKey, first.Caller->GetName(),
                             {{"a", first.Target->GetName()}, {"b", second.Target->GetName()}});
}

void Run(int adminSlot, int targetSlot, int param, const ParamAction& action)
{
    ActionDispatcher{}.Run(adminSlot, targetSlot, param, action);
}

}  // namespace AdminSystem::Admin::Actions
