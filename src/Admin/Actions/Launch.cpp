#include "Launch.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "ActionContext.hpp"

#include <CS2Kit/Core/Scheduler.hpp>
#include <CS2Kit/Sdk/Entity.hpp>
#include <mathlib/vector.h>
#include <random>

using CS2Kit::Core::Kit;

namespace AdminSystem::Admin::Actions
{

using namespace CS2Kit::Core;
using namespace CS2Kit::Sdk;

namespace
{
constexpr float LaunchUpward = 800.0f;
constexpr float LaunchHorizontal = 100.0f;
constexpr int FallProtectMs = 3000;

float Rand(float lo, float hi)
{
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(rng);
}
}  // namespace

void DoLaunch(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 's');
    if (!ctx.Valid() || !ctx.TargetCtrl.IsAlive())
        return;

    // Write velocity directly on the pawn rather than through the Teleport vfunc.
    // Teleport(nullptr origin, ...) was crashing the server in CS2 builds we tested;
    // m_vecAbsVelocity is the conventional path for velocity-only changes.
    Vector vel{Rand(-LaunchHorizontal, LaunchHorizontal), Rand(-LaunchHorizontal, LaunchHorizontal), LaunchUpward};
    ctx.TargetCtrl.SetPawnField<Vector>("CBaseEntity", "m_vecAbsVelocity", vel);

    // FL_GODMODE on m_fFlags is the working CS2 invincibility path (legacy m_takedamage is no-op).
    auto savedFlags = ctx.TargetCtrl.GetPawnField<uint32_t>("CBaseEntity", "m_fFlags");
    ctx.TargetCtrl.SetPawnField<uint32_t>("CBaseEntity", "m_fFlags", savedFlags | CS2Kit::Sdk::FL_GODMODE);

    int slot = targetSlot;

    Kit().Scheduler.Delay(FallProtectMs, [slot]() {
        PlayerController pc(slot);
        if (!pc.IsValid())
        {
            return;
        }

        auto flags = pc.GetPawnField<uint32_t>("CBaseEntity", "m_fFlags");
        pc.SetPawnField<uint32_t>("CBaseEntity", "m_fFlags", flags & ~CS2Kit::Sdk::FL_GODMODE);
    });

    Broadcast(ctx, "broadcast.launched");
}

}  // namespace AdminSystem::Admin::Actions
