#include "Admin/Actions/Descriptors.hpp"

namespace AdminSystem::Admin::Actions
{

const ParamAction ChangeTeam{Permission::Control, /*requireAlive*/ false,
                             [](const ActionContext& ctx, int team) -> OptKey {
                                 if (!ctx.Target().Controller().ChangeTeam(static_cast<VoltMod::Team>(team)))
                                 {
                                     return std::nullopt;
                                 }
                                 return "broadcast.teamChanged";
                             }};

}  // namespace AdminSystem::Admin::Actions
