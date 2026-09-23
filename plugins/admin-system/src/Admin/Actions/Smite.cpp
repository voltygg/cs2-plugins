#include "Admin/Actions/Descriptors.hpp"

#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Actions
{

static constexpr int SmiteSlayDelayMs = 300;
static constexpr float ExplosionCleanupSeconds = 1.0f;
// env_explosion spawnflag 1 = No Damage: the fireball and boom play, the kill
// below stays deterministic, and bystanders are untouched. Flag 64 would mute it.
static constexpr int EnvExplosionNoDamage = 1;

Action MakeSmite(VoltMod::Runtime& runtime, PawnTimers& timers)
{
    return Action{Permission::Fun, /*requireAlive*/ true, [&runtime, &timers](const ActionContext& ctx) -> OptKey {
                      VoltMod::KeyValues kv;
                      kv.Set("origin", ctx.Target().Pawn().Origin()).Set("spawnflags", EnvExplosionNoDamage);
                      if (const VoltMod::Entity boom = runtime.Entities.Spawn("env_explosion", kv))
                      {
                          boom.AcceptInput("Explode");
                          boom.RemoveAfter(ExplosionCleanupSeconds);
                      }

                      // Delayed so the blast plays before the target drops.
                      timers.SlayAfter(ctx.Target().Slot(), SmiteSlayDelayMs);
                      return "broadcast.smote";
                  }};
}

}  // namespace AdminSystem::Admin::Actions
