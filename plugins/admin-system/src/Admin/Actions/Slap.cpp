#include "Descriptors.hpp"

#include <CS2Kit/Sdk/PawnOps.hpp>

namespace AdminSystem::Admin::Actions
{

const Action Slap{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      CS2Kit::Sdk::PawnOps::Slap(ctx.TargetCtrl);
                      return "broadcast.slapped";
                  }};

}  // namespace AdminSystem::Admin::Actions
