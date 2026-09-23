#include "Admin/Actions/Descriptors.hpp"

namespace AdminSystem::Admin::Actions
{

Action MakeSlap(PawnTimers& timers)
{
    return Action{Permission::Control, /*requireAlive*/ true, [&timers](const ActionContext& ctx) -> OptKey {
                      timers.Slap(ctx.Target().Pawn());
                      return "broadcast.slapped";
                  }};
}

}  // namespace AdminSystem::Admin::Actions
