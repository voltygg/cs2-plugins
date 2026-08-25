#include "Descriptors.hpp"

#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Actions
{

const Action Slap{Flag(Permission::Control), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      ctx.Rt.Pawns.Slap(ctx.TargetCtrl);
                      return "broadcast.slapped";
                  }};

}  // namespace AdminSystem::Admin::Actions
