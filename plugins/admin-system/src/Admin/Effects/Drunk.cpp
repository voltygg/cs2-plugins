#include "Descriptors.hpp"
#include "EffectRegistry.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <VoltMod/Sdk/Messaging/UserMessage.hpp>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;

namespace
{

// Re-shaken faster than each shake decays, so the view never fully settles between ticks.
constexpr int DrunkIntervalMs = 700;
constexpr int DrunkDurationSec = 20;
constexpr float ShakeDurationSec = 1.2f;
constexpr float ShakeFrequency = 1.5f;  // slow enough to read as a sway rather than a rattle
constexpr float ShakeAmplitude = 8.0f;

}  // namespace

/**
 * Screen-sway on the target. Uses the engine's own shake user message rather than writing punch
 * angles: nothing in CS2 reliably drives m_aimPunchAngle server-side, and the shake message is
 * both supported and client-predicted.
 */
const Effect Drunk{.Permission = Flag(Permission::Fun),
                   .Id = static_cast<int>(EffectId::Drunk),
                   .NameKey = "action.drunk",
                   .OnKey = "broadcast.drunkOn",
                   .OffKey = "broadcast.drunkOff",
                   .Scope = EffectScope::Round,
                   .TickIntervalMs = DrunkIntervalMs,
                   .DurationMs = DrunkDurationSec * 1000,
                   .RequireAlive = true,
                   .Setup = [](const ActionContext& ctx) -> EffectInstance {
                       int slot = ctx.Target->GetSlot();
                       auto& messages = ctx.Rt.Messages;
                       auto& entities = ctx.Rt.Entities;
                       return {.OnTick = [&messages, &entities, slot]() {
                           // Shaking a dead or departed player would be wasted traffic.
                           auto controller = entities.Controller(slot);
                           if (controller.IsValid() && controller.IsAlive())
                               messages.Shake(slot, ShakeDurationSec, ShakeFrequency, ShakeAmplitude);
                       }};
                   }};

}  // namespace AdminSystem::Admin::Effects
