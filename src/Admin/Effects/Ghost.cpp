#include "Ghost.hpp"

#include "../Actions/ActionContext.hpp"
#include "EffectManager.hpp"

namespace AdminSystem::Admin::Effects
{

using AdminSystem::Admin::Actions::ActionContext;
using AdminSystem::Admin::Actions::Broadcast;
using AdminSystem::Admin::Actions::Resolve;
using CS2Kit::Sdk::PlayerController;

void ToggleGhost(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 'f');
    if (!ctx.Valid())
        return;

    auto& mgr = EffectManager::Instance();
    if (mgr.IsActive(targetSlot, EffectId::Ghost))
    {
        mgr.Cancel(targetSlot, EffectId::Ghost);
        Broadcast(ctx, "broadcastGhostOff");
        return;
    }

    ctx.TargetCtrl.SetVisible(false);

    int slot = targetSlot;
    auto cancel = [slot]() {
        PlayerController pc(slot);
        if (pc.IsValid())
            pc.SetVisible(true);
    };

    mgr.Apply(targetSlot, EffectId::Ghost, /*timerHandle*/ 0, std::move(cancel), /*roundScoped*/ false);
    Broadcast(ctx, "broadcastGhostOn");
}

}  // namespace AdminSystem::Admin::Effects
