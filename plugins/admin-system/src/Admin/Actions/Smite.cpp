#include "Descriptors.hpp"

#include <CS2Kit/Core/Scheduler.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Sdk/EntityKeyValues.hpp>
#include <mathlib/vector.h>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Actions
{

constexpr int SmiteSlayDelayMs = 300;
constexpr float ExplosionCleanupSeconds = 1.0f;
// env_explosion spawnflag 1 = No Damage: the fireball and boom play, the kill
// below stays deterministic, and bystanders are untouched. Flag 64 would mute it.
constexpr int EnvExplosionNoDamage = 1;

const Action Smite{Flag(Permission::Fun), /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                       auto& ops = Engine().EntityOps;
                       if (ops.CanSpawn())
                       {
                           CS2Kit::Sdk::EntityKeyValues kv;
                           kv.Set("origin", ctx.TargetCtrl.GetAbsOrigin()).Set("spawnflags", EnvExplosionNoDamage);
                           if (auto* boom = ops.Spawn("env_explosion", kv))
                           {
                               ops.AcceptInput(boom, "Explode");
                               ops.RemoveDelayed(boom, ExplosionCleanupSeconds);
                           }
                       }

                       int slot = ctx.Target->GetSlot();
                       Engine().Scheduler.Delay(SmiteSlayDelayMs, [slot]() {
                           CS2Kit::Sdk::PlayerController pc(slot);
                           if (pc.IsValid() && pc.IsAlive())
                               pc.Slay();
                       });
                       return "broadcast.smote";
                   }};

}  // namespace AdminSystem::Admin::Actions
