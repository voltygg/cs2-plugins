#include "Descriptors.hpp"

#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Actions
{

Action MakeSlap(VoltMod::Runtime& runtime)
{
    return Action{Flag(Permission::Control), /*requireAlive*/ true, [&runtime](const ActionContext& ctx) -> OptKey {
                      runtime.Pawns.Slap(ctx.TargetPawn());
                      return "broadcast.slapped";
                  }};
}

}  // namespace AdminSystem::Admin::Actions
