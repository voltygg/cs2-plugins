#include "Ghost.hpp"

#include "../Actions/ActionContext.hpp"
#include "EffectManager.hpp"

#include <cstdint>

namespace AdminSystem::Admin::Effects
{

using AdminSystem::Admin::Actions::ActionContext;
using AdminSystem::Admin::Actions::Broadcast;
using AdminSystem::Admin::Actions::Resolve;

namespace
{
// CS2 RenderMode_t values (legacy Source numbering): 0 = normal, 3 = transparent texture.
constexpr uint8_t RenderModeNormal = 0;
constexpr uint8_t RenderModeTransTexture = 3;

// Color is stored as a 32-bit RGBA value, low byte = R.
constexpr uint32_t ColorOpaqueWhite = 0xFFFFFFFFu;
constexpr uint32_t ColorGhost = 0x80FFFFFFu;  // 50% alpha white
}  // namespace

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

    auto savedMode = ctx.TargetCtrl.GetPawnField<uint8_t>("CBaseModelEntity", "m_nRenderMode");
    auto savedColor = ctx.TargetCtrl.GetPawnField<uint32_t>("CBaseModelEntity", "m_clrRender");

    ctx.TargetCtrl.SetPawnField<uint8_t>("CBaseModelEntity", "m_nRenderMode", RenderModeTransTexture);
    ctx.TargetCtrl.SetPawnField<uint32_t>("CBaseModelEntity", "m_clrRender", ColorGhost);

    // The cancel callback needs to read the target pawn at the time it runs, not the
    // saved-pointer. Capture the slot and rebuild the controller — the pawn may have
    // changed across deaths, in which case the restore is a no-op.
    int slot = targetSlot;
    auto cancel = [slot, savedMode, savedColor]() {
        CS2Kit::Sdk::PlayerController pc(slot);
        if (!pc.IsValid())
            return;

        pc.SetPawnField<uint8_t>("CBaseModelEntity", "m_nRenderMode", savedMode == 0 ? RenderModeNormal : savedMode);
        pc.SetPawnField<uint32_t>("CBaseModelEntity", "m_clrRender", savedColor == 0 ? ColorOpaqueWhite : savedColor);
    };

    mgr.Apply(targetSlot, EffectId::Ghost, /*timerHandle*/ 0, std::move(cancel), /*roundScoped*/ false);
    Broadcast(ctx, "broadcastGhostOn");
}

}  // namespace AdminSystem::Admin::Effects
