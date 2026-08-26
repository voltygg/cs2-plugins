#include "Descriptors.hpp"

#include <VoltMod/Hooks/GlowVision.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;
using VoltMod::GlowVision;

Effect MakeWallhack(VoltMod::Runtime& runtime)
{
    return Effect{.Permission = Flag(Permission::Wallhack),
                  .Id = static_cast<int>(EffectId::Wallhack),
                  .NameKey = "action.wallhack",
                  .OnKey = "broadcast.wallhackOn",
                  .OffKey = "broadcast.wallhackOff",
                  .Scope = EffectScope::Round,
                  .TickIntervalMs = GlowVision::ReconcileIntervalMs,
                  .Setup = [&runtime](const ActionContext& ctx, int) -> EffectInstance {
                      auto glow = runtime.Visibility.CreateGlow(ctx.Target().Slot());

                      // Build the glow clones immediately; the repeating tick then tracks spawns/deaths/team
                      // changes. OnStop clears the transmit-filter entries and removes any surviving clones
                      // (on a round restart the props are already gone).
                      glow->Reconcile();

                      return {.OnTick = [glow]() { glow->Reconcile(); }, .OnStop = [glow]() { glow->Destroy(); }};
                  }};
}

}  // namespace AdminSystem::Admin::Effects
