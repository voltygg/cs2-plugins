#include "Descriptors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;

// Bhop is provided by the separate bhop plugin; the grant crosses the module boundary as a
// server command. With the bhop plugin absent (or in "enabled" mode, where everyone already
// has bhop) the toggle is harmless - the engine logs "Unknown command" / the grant is a no-op.

Effect MakeBhop(VoltMod::Runtime& runtime)
{
    return Effect{.Permission = Flag(Permission::Bhop),
                  .Id = static_cast<int>(EffectId::Bhop),
                  .NameKey = "action.bhop",
                  .OnKey = "broadcast.bhopOn",
                  .OffKey = "broadcast.bhopOff",
                  .Scope = EffectScope::Session,  // session grant: survives the death sweep
                  .Setup = [&runtime](const ActionContext& ctx, int) -> EffectInstance {
                      int64_t steamId = ctx.Target().SteamId();
                      auto& conVars = runtime.ConVars;
                      conVars.ExecuteServerCommand(std::format("bhop_player {} 1", steamId));
                      // The effect manager is a plugin member, so it is torn down before the
                      // runtime - this reference outlives every OnStop it can reach.
                      return {.OnStop = [&conVars, steamId]() {
                          conVars.ExecuteServerCommand(std::format("bhop_player {} 0", steamId));
                      }};
                  }};
}

}  // namespace AdminSystem::Admin::Effects
