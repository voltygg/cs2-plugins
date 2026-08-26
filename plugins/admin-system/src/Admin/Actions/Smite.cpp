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
                           kv.Set("origin", ctx.TargetCtrl.GetAbsOrigin()).Set("spawnflags", EnvExplosionNoDamage);
                           if (auto* boom = ops.Spawn("env_explosion", kv))
                           {
                               ops.AcceptInput(boom, "Explode");
                               ops.RemoveDelayed(boom, ExplosionCleanupSeconds);
                           }
                       }

                       int slot = ctx.Target->GetSlot();
                       ctx.Rt.Scheduler.Delay(SmiteSlayDelayMs, [&entities = ctx.Rt.Entities, slot]() {
                           VoltMod::PlayerController pc = entities.Controller(slot);
                           if (pc.IsValid() && pc.IsAlive())
                               pc.Slay();
                       });
                       return "broadcast.smote";
                   }};

}  // namespace AdminSystem::Admin::Actions
