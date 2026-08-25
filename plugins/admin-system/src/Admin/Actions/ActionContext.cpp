#include "ActionContext.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"

namespace AdminSystem::Admin::Actions
{

void Broadcast(App& app, const ActionContext& first, const ActionContext& second, const std::string& translationKey)
{
    if (!first.Caller || !first.Target || !second.Target)
        return;
    app.Chat.BroadcastAction(translationKey, first.Caller->GetName(),
                             {{"a", first.Target->GetName()}, {"b", second.Target->GetName()}});
}

}  // namespace AdminSystem::Admin::Actions
