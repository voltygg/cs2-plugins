#include "ActionContext.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"

namespace AdminSystem::Admin::Actions
{

// The dispatcher holds only the runtime reference - policy comes from Runtime.Policy, set once
// in OnLoad - so building one per call costs nothing.
using VoltMod::Players::ActionDispatcher;

ActionContext Resolve(VoltMod::Runtime& runtime, int adminSlot, int targetSlot, const std::string& requiredFlag)
{
    return ActionDispatcher{runtime}.Resolve(adminSlot, targetSlot, requiredFlag);
}

void Broadcast(App& app, const ActionContext& first, const ActionContext& second, const std::string& translationKey)
{
    if (!first.Caller || !first.Target || !second.Target)
        return;
    app.Chat.BroadcastAction(translationKey, first.Caller->GetName(),
                             {{"a", first.Target->GetName()}, {"b", second.Target->GetName()}});
}

void Run(VoltMod::Runtime& runtime, int adminSlot, int targetSlot, int param, const ParamAction& action)
{
    ActionDispatcher{runtime}.Run(adminSlot, targetSlot, param, action);
}

}  // namespace AdminSystem::Admin::Actions
