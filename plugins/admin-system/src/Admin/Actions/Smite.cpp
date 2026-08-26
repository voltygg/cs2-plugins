#include "Descriptors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Scheduler.hpp>
#include <VoltMod/Entities/KeyValues.hpp>
#include <VoltMod/Runtime.hpp>
#include <mathlib/vector.h>

namespace AdminSystem::Admin::Actions
{

constexpr int SmiteSlayDelayMs = 300;
constexpr float ExplosionCleanupSeconds = 1.0f;
// env_explosion spawnflag 1 = No Damage: the fireball and boom play, the kill
// below stays deterministic, and bystanders are untouched. Flag 64 would mute it.
constexpr int EnvExplosionNoDamage = 1;

const Action Smite{Flag(Permission::Fun), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                       auto& ops = ctx.Rt.EntityOps;
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
                       ctx.Rt.Pawns.SlayDelayed(ctx.Target->GetSlot(), SmiteSlayDelayMs);
                       return "broadcast.smote";
                   }};

}  // namespace AdminSystem::Admin::Actions
