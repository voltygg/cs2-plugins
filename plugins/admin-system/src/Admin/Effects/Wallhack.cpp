#include "Descriptors.hpp"
#include "EffectRegistry.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Hooks/GlowVision.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;
using VoltMod::GlowVision;

const Effect Wallhack{.Permission = Flag(Permission::Wallhack),
                      .Id = static_cast<int>(EffectId::Wallhack),
                      .NameKey = "action.wallhack",
                      .OnKey = "broadcast.wallhackOn",
                      .OffKey = "broadcast.wallhackOff",
                      .Scope = EffectScope::Round,
                      .TickIntervalMs = GlowVision::ReconcileIntervalMs,
                      .Setup = [](const ActionContext& ctx) -> EffectInstance {
                          auto glow = ctx.Rt.Visibility.CreateGlow(ctx.Target->GetSlot());

                          // Build the glow clones immediately; the repeating tick then tracks spawns/deaths/team
                          // changes. OnStop clears the transmit-filter entries and removes any surviving clones
                          // (on a round restart the props are already gone).
                          glow->Reconcile();

                          return {.OnTick = [glow]() { glow->Reconcile(); }, .OnStop = [glow]() { glow->Destroy(); }};
                      }};

}  // namespace AdminSystem::Admin::Effects
