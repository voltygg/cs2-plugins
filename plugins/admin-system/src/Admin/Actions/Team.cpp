#include "Descriptors.hpp"

#include <CS2Kit/Sdk/PawnOps.hpp>

namespace AdminSystem::Admin::Actions
{

const ParamAction ChangeTeam{Flag(Permission::Control), /*requireAlive*/ false,
                             [](const ActionContext& ctx, int team) -> OptKey {
                                 if (!CS2Kit::Sdk::PawnOps::ChangeTeamSafe(ctx.TargetCtrl, team))
                                     return std::nullopt;
                                 return "broadcast.teamChanged";
                             }};

}  // namespace AdminSystem::Admin::Actions
