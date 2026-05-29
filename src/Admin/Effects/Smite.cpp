#include "Smite.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../Actions/ActionContext.hpp"

#include <CS2Kit/Core/Scheduler.hpp>

namespace AdminSystem::Admin::Effects
{

using AdminSystem::Admin::Actions::Broadcast;
using AdminSystem::Admin::Actions::Resolve;
using CS2Kit::Core::Scheduler;

void DoSmite(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 'f');
    if (!ctx.Valid() || !ctx.TargetCtrl.IsAlive())
        return;

    Broadcast(ctx, "broadcast.smote");

    int slot = targetSlot;
    CS2Kit::Core::Kit().Scheduler.Delay(250, [slot]() {
        CS2Kit::Sdk::PlayerController pc(slot);
        if (pc.IsValid() && pc.IsAlive())
            pc.Slay();
    });
}

}  // namespace AdminSystem::Admin::Effects
