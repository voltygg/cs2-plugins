#include "Descriptors.hpp"

namespace AdminSystem::Admin::Actions
{

const ParamAction ChangeTeam{Permission::Control, /*requireAlive*/ false,
                             [](const ActionContext& ctx, int team) -> OptKey {
                                 if (team < TeamSpec || team > TeamCt)
                                     return std::nullopt;
                                 ctx.TargetCtrl.ChangeTeam(team);
                                 return "broadcast.teamChanged";
                             }};

}  // namespace AdminSystem::Admin::Actions
