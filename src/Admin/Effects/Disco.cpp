#include "Disco.hpp"

#include "../Actions/ActionContext.hpp"
#include "EffectManager.hpp"

#include <CS2Kit/Core/Scheduler.hpp>
#include <array>
#include <cstdint>
#include <memory>

namespace AdminSystem::Admin::Effects
{

using AdminSystem::Admin::Actions::Broadcast;
using AdminSystem::Admin::Actions::Resolve;
using CS2Kit::Core::Scheduler;

namespace
{
constexpr uint8_t RenderModeNormal = 0;
constexpr uint8_t RenderModeTransTexture = 3;
constexpr uint32_t ColorOpaqueWhite = 0xFFFFFFFFu;

// Bright RGBA values cycled at 200 ms — red / orange / yellow / green / blue / magenta.
constexpr std::array<uint32_t, 6> Palette = {
    0xFF0000FFu, 0xFF8000FFu, 0xFFFF00FFu, 0x00FF00FFu, 0x0000FFFFu, 0xFF00FFFFu,
};

constexpr int DiscoIntervalMs = 200;
constexpr int DiscoDurationSec = 15;
}  // namespace

void ToggleDisco(int adminSlot, int targetSlot)
{
    auto ctx = Resolve(adminSlot, targetSlot, 'f');
    if (!ctx.Valid())
        return;

    auto& mgr = EffectManager::Instance();
    if (mgr.IsActive(targetSlot, EffectId::Disco))
    {
        mgr.Cancel(targetSlot, EffectId::Disco);
        Broadcast(ctx, "broadcast.discoOff");
        return;
    }

    auto savedMode = ctx.TargetCtrl.GetPawnField<uint8_t>("CBaseModelEntity", "m_nRenderMode");
    auto savedColor = ctx.TargetCtrl.GetPawnField<uint32_t>("CBaseModelEntity", "m_clrRender");

    int slot = targetSlot;
    auto idx = std::make_shared<size_t>(0);
    uint64_t timer = Scheduler::Instance().Repeat(DiscoIntervalMs, [slot, idx]() {
        CS2Kit::Sdk::PlayerController pc(slot);
        if (!pc.IsValid() || !pc.IsAlive())
            return;
        pc.SetPawnField<uint8_t>("CBaseModelEntity", "m_nRenderMode", RenderModeTransTexture);
        pc.SetPawnField<uint32_t>("CBaseModelEntity", "m_clrRender", Palette[*idx]);
        *idx = (*idx + 1) % Palette.size();
    });

    auto cancel = [slot, savedMode, savedColor]() {
        CS2Kit::Sdk::PlayerController pc(slot);
        if (!pc.IsValid())
            return;
        pc.SetPawnField<uint8_t>("CBaseModelEntity", "m_nRenderMode",
                                 savedMode == 0 ? RenderModeNormal : savedMode);
        pc.SetPawnField<uint32_t>("CBaseModelEntity", "m_clrRender",
                                  savedColor == 0 ? ColorOpaqueWhite : savedColor);
    };

    mgr.Apply(targetSlot, EffectId::Disco, timer, std::move(cancel), /*roundScoped*/ true);

    Scheduler::Instance().Delay(DiscoDurationSec * 1000, [slot]() {
        EffectManager::Instance().Cancel(slot, EffectId::Disco);
    });

    Broadcast(ctx, "broadcast.discoOn");
}

}  // namespace AdminSystem::Admin::Effects
