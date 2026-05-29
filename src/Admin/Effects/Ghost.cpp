#include "Ghost.hpp"
#include "../../Core/Managers.hpp"

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

    bool on = Sys().Effects.Toggle(targetSlot, EffectId::Ghost, [&]() -> EffectSetup {
        ctx.TargetCtrl.SetVisible(false);
        int slot = targetSlot;
        return {0, [slot]() {
                    PlayerController pc(slot);
                    if (pc.IsValid())
                        pc.SetVisible(true);
                }, false};
    });

    Broadcast(ctx, on ? "broadcast.ghostOn" : "broadcast.ghostOff");
}

}  // namespace AdminSystem::Admin::Effects
