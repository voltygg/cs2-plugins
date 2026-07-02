#include "Descriptors.hpp"

#include <CS2Kit/Core/Scheduler.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Sdk/Entity.hpp>
#include <mathlib/vector.h>
#include <random>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Actions
{

using namespace CS2Kit::Core;
using namespace CS2Kit::Sdk;

namespace
{
constexpr float SlapUpward = 800.0f;
constexpr float SlapHorizontal = 100.0f;
constexpr int FallProtectMs = 3000;

float Rand(float lo, float hi)
{
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(rng);
}
}  // namespace

const Action Slap{Permission::Control, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                      // Write velocity directly on the pawn rather than through the Teleport vfunc.
                      // Teleport(nullptr origin, ...) was crashing the server in CS2 builds we tested;
                      // m_vecAbsVelocity is the conventional path for velocity-only changes.
                      ctx.TargetCtrl.SetVelocity(
                          {Rand(-SlapHorizontal, SlapHorizontal), Rand(-SlapHorizontal, SlapHorizontal), SlapUpward});

                      // FL_GODMODE on m_fFlags is the working CS2 invincibility path (legacy m_takedamage is no-op).
                      // Only toggle it for fall protection if the target wasn't already in godmode, otherwise the
                      // delayed clear below would silently strip an admin-applied Godmode.
                      uint32_t flags = ctx.TargetCtrl.GetFlags();
                      if ((flags & CS2Kit::Sdk::FL_GODMODE) == 0)
                      {
                          ctx.TargetCtrl.SetFlags(flags | CS2Kit::Sdk::FL_GODMODE);

                          int slot = ctx.Target->GetSlot();
                          Engine().Scheduler.Delay(FallProtectMs, [slot]() {
                              PlayerController pc(slot);
                              if (pc.IsValid())
                                  pc.SetFlags(pc.GetFlags() & ~CS2Kit::Sdk::FL_GODMODE);
                          });
                      }

                      return "broadcast.slapped";
                  }};

}  // namespace AdminSystem::Admin::Actions
