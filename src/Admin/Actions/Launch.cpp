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
    RunAction(adminSlot, targetSlot, 's', [targetSlot](const ActionContext& ctx) -> std::optional<std::string> {
        if (!ctx.TargetCtrl.IsAlive())
            return std::nullopt;

        // Write velocity directly on the pawn rather than through the Teleport vfunc.
        // Teleport(nullptr origin, ...) was crashing the server in CS2 builds we tested;
        // m_vecAbsVelocity is the conventional path for velocity-only changes.
        ctx.TargetCtrl.SetVelocity({Rand(-LaunchHorizontal, LaunchHorizontal),
                                    Rand(-LaunchHorizontal, LaunchHorizontal), LaunchUpward});

        // FL_GODMODE on m_fFlags is the working CS2 invincibility path (legacy m_takedamage is no-op).
        ctx.TargetCtrl.SetFlags(ctx.TargetCtrl.GetFlags() | FL_GODMODE);

        int slot = targetSlot;
        Kit().Scheduler.Delay(FallProtectMs, [slot]() {
            PlayerController pc(slot);
            if (pc.IsValid())
                pc.SetFlags(pc.GetFlags() & ~FL_GODMODE);
        });

        return "broadcast.launched";
    });
}

}  // namespace AdminSystem::Admin::Actions
