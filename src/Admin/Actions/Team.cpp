#include "Team.hpp"

#include "ActionContext.hpp"

namespace AdminSystem::Admin::Actions
{

void DoChangeTeam(int adminSlot, int targetSlot, int team)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid())
        return;
    if (team < TeamSpec || team > TeamCt)
        return;
    ctx.TargetCtrl.ChangeTeam(team);
    Broadcast(ctx, "broadcastTeamChanged");
}

}  // namespace AdminSystem::Admin::Actions
