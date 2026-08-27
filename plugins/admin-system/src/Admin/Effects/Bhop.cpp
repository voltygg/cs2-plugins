#include "Descriptors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;

// The separate bhop plugin exposes grants through a server command.

Effect MakeBhop(VoltMod::Runtime& runtime)
{
    return Effect{.Permission = Flag(Permission::Bhop),
                  .Id = static_cast<int>(EffectId::Bhop),
                  .NameKey = "action.bhop",
                  .OnKey = "broadcast.bhopOn",
                  .OffKey = "broadcast.bhopOff",
                  .Scope = EffectScope::Session,  // Survives death.
                  .Setup = [&runtime](const ActionContext& ctx, int) -> EffectInstance {
                      int64_t steamId = ctx.Target().SteamId();
                      // There is no recovery path if the engine is already unavailable.
                      auto& conVars = runtime.ConVars;
                      (void)conVars.ExecuteServerCommand(std::format("bhop_player {} 1", steamId));
                      // The runtime outlives the effect manager and its callbacks.
                      return {.OnStop = [&conVars, steamId]() {
                          (void)conVars.ExecuteServerCommand(std::format("bhop_player {} 0", steamId));
                      }};
                  }};
}

}  // namespace AdminSystem::Admin::Effects
