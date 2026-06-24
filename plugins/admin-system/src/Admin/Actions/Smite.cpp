#include "Descriptors.hpp"
#include <CS2Kit/Core/Services.hpp>

#include <CS2Kit/Core/Scheduler.hpp>

using CS2Kit::Core::Kit;

namespace AdminSystem::Admin::Actions
{

constexpr int SmiteDelayMs = 250;

const Action Smite{Permission::Fun, /*requireAlive*/ true, [](const ActionContext& ctx) -> OptKey {
                       int slot = ctx.Target->GetSlot();
                       Kit().Scheduler.Delay(SmiteDelayMs, [slot]() {
                           CS2Kit::Sdk::PlayerController pc(slot);
                           if (pc.IsValid() && pc.IsAlive())
                               pc.Slay();
                       });
                       return "broadcast.smote";
                   }};

}  // namespace AdminSystem::Admin::Actions
