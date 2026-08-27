#include "Descriptors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Scheduler.hpp>
#include <VoltMod/Entities/KeyValues.hpp>
#include <VoltMod/Runtime.hpp>
#include <mathlib/vector.h>

namespace AdminSystem::Admin::Actions
{

static constexpr int SmiteSlayDelayMs = 300;
static constexpr float ExplosionCleanupSeconds = 1.0f;
// env_explosion spawnflag 1 = No Damage: the fireball and boom play, the kill
// below stays deterministic, and bystanders are untouched. Flag 64 would mute it.
static constexpr int EnvExplosionNoDamage = 1;

Action MakeSmite(VoltMod::Runtime& runtime)
{
    return Action{Flag(Permission::Fun), /*requireAlive*/ true, [&runtime](const ActionContext& ctx) -> OptKey {
                      auto& ops = runtime.World.EntityOps;
                      if (ops.CanSpawn())
                      {
                          VoltMod::KeyValues kv;
                          kv.Set("origin", ctx.TargetPawn().Origin()).Set("spawnflags", EnvExplosionNoDamage);
                          if (auto* boom = ops.Spawn("env_explosion", kv))
                          {
                              ops.AcceptInput(boom, "Explode");
                              ops.RemoveDelayed(boom, ExplosionCleanupSeconds);
                          }
                      }

                      // Delayed so the blast plays before the target drops; Pawns owns the timer,
                      // which is what keeps it off the next occupant of the slot.
                      runtime.World.Pawns.SlayDelayed(ctx.Target().Slot(), SmiteSlayDelayMs);
                      return "broadcast.smote";
                  }};
}

}  // namespace AdminSystem::Admin::Actions
