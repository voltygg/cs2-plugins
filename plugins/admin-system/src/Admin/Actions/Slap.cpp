#include "Descriptors.hpp"

#include <VoltMod/Runtime.hpp>
#include <VoltMod/Sdk/Entity/PawnOps.hpp>

namespace AdminSystem::Admin::Actions
{

const Action Slap{Flag(Permission::Control), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      VoltMod::Sdk::PawnOps::Slap(ctx.TargetCtrl, ctx.Rt.Scheduler, ctx.Rt.Slots);
                      return "broadcast.slapped";
                  }};

}  // namespace AdminSystem::Admin::Actions
