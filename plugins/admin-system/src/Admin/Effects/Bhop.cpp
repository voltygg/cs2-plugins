#include "Descriptors.hpp"
#include "EffectRegistry.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <format>

using CS2Kit::Core::Engine;
using CS2Kit::Core::EngineOrNull;

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;

// Bhop is provided by the separate bhop plugin; the grant crosses the module boundary as a
// server command. With the bhop plugin absent (or in "enabled" mode, where everyone already
// has bhop) the toggle is harmless - the engine logs "Unknown command" / the grant is a no-op.

const Effect Bhop{.Permission = Flag(Permission::Bhop),
                  .Id = static_cast<int>(EffectId::Bhop),
                  .NameKey = "action.bhop",
                  .OnKey = "broadcast.bhopOn",
                  .OffKey = "broadcast.bhopOff",
                  .Scope = EffectScope::Session,  // session grant: survives the death sweep
                  .Setup = [](const ActionContext& ctx) -> EffectInstance {
                      int64_t steamId = ctx.Target->GetSteamID();
                      Engine().ConVars.ExecuteServerCommand(std::format("bhop_player {} 1", steamId).c_str());
                      return {.OnStop = [steamId]() {
                          if (auto* engine = EngineOrNull())
                              engine->ConVars.ExecuteServerCommand(std::format("bhop_player {} 0", steamId).c_str());
                      }};
                  }};

static const bool _registered = CS2Kit::Registry<EffectEntry>::Add({.Order = 50, .Toggle = &Bhop});

}  // namespace AdminSystem::Admin::Effects
