#include "Descriptors.hpp"
#include "EffectRegistry.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Sdk/GlowVision.hpp>
#include <memory>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;
using CS2Kit::Sdk::GlowVision;

const Effect Wallhack{.Permission = Flag(Permission::Wallhack),
                      .Id = static_cast<int>(EffectId::Wallhack),
                      .NameKey = "action.wallhack",
                      .OnKey = "broadcast.wallhackOn",
                      .OffKey = "broadcast.wallhackOff",
                      .Scope = EffectScope::Round,
                      .TickIntervalMs = GlowVision::ReconcileIntervalMs,
                      .Setup = [](const ActionContext& ctx) -> EffectInstance {
                          auto glow = std::make_shared<GlowVision>(ctx.Target->GetSlot());

                          // Build the glow clones immediately; the repeating tick then tracks spawns/deaths/team
                          // changes. OnStop clears the transmit-filter entries and removes any surviving clones
                          // (on a round restart the props are already gone).
                          glow->Reconcile();

                          return {.OnTick = [glow]() { glow->Reconcile(); }, .OnStop = [glow]() { glow->Destroy(); }};
                      }};

}  // namespace AdminSystem::Admin::Effects
