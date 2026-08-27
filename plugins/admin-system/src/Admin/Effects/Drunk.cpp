#include "Descriptors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Messaging/Messages.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;

// Re-shaken faster than each shake decays, so the view never fully settles between ticks.
static constexpr int DrunkIntervalMs = 700;
static constexpr int DrunkDurationSec = 20;
static constexpr float ShakeDurationSec = 1.2f;
static constexpr float ShakeFrequency = 1.5f;  // slow enough to read as a sway rather than a rattle
static constexpr float ShakeAmplitude = 8.0f;

/**
 * Screen-sway on the target. Uses the engine's own shake user message rather than writing punch
 * angles: nothing in CS2 reliably drives m_aimPunchAngle server-side, and the shake message is
 * both supported and client-predicted.
 */
Effect MakeDrunk(VoltMod::Runtime& runtime)
{
    return Effect{.Permission = Flag(Permission::Fun),
                  .Id = static_cast<int>(EffectId::Drunk),
                  .NameKey = "action.drunk",
                  .OnKey = "broadcast.drunkOn",
                  .OffKey = "broadcast.drunkOff",
                  .Scope = EffectScope::Round,
                  .TickIntervalMs = DrunkIntervalMs,
                  .DurationMs = DrunkDurationSec * 1000,
                  .RequireAlive = true,
                  .Setup = [&runtime](const ActionContext& ctx, int) -> EffectInstance {
                      int slot = ctx.Target().Slot();
                      auto& messages = runtime.Messages;
                      auto& entities = runtime.Entities;
                      return {.OnTick = [&messages, &entities, slot]() {
                          // Shaking a dead or departed player would be wasted traffic.
                          auto pawn = entities.PawnOf(slot);
                          if (pawn && pawn.IsAlive())
                              messages.Shake(slot, ShakeDurationSec, ShakeFrequency, ShakeAmplitude);
                      }};
                  }};
}

}  // namespace AdminSystem::Admin::Effects
