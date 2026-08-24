#include "Descriptors.hpp"

#include <VoltMod/Sdk/PawnOps.hpp>

namespace AdminSystem::Admin::Actions
{

const Action Slap{Flag(Permission::Control), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      VoltMod::Sdk::PawnOps::Slap(ctx.TargetCtrl);
                      return "broadcast.slapped";
                  }};

}  // namespace AdminSystem::Admin::Actions
