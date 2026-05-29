#include "Team.hpp"

#include "ActionContext.hpp"

namespace AdminSystem::Admin::Actions
{

void DoChangeTeam(int adminSlot, int targetSlot, int team)
{
    RunAction(adminSlot, targetSlot, 's', [team](const ActionContext& ctx) -> std::optional<std::string> {
        if (team < TeamSpec || team > TeamCt)
            return std::nullopt;
        ctx.TargetCtrl.ChangeTeam(team);
        return "broadcast.teamChanged";
    });
}

}  // namespace AdminSystem::Admin::Actions
