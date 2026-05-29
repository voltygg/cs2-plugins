#include "Disco.hpp"
#include "../../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../Actions/ActionContext.hpp"
#include "EffectManager.hpp"

#include <CS2Kit/Core/Scheduler.hpp>
#include <array>
#include <cstdint>
#include <memory>

using CS2Kit::Core::Kit;

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

    bool on = Sys().Effects.Toggle(targetSlot, EffectId::Disco, [&]() -> EffectSetup {
        uint8_t savedMode = ctx.TargetCtrl.GetRenderMode();
        uint32_t savedColor = ctx.TargetCtrl.GetRenderColor();

        int slot = targetSlot;
        auto idx = std::make_shared<size_t>(0);
        uint64_t timer = Kit().Scheduler.Repeat(DiscoIntervalMs, [slot, idx]() {
            CS2Kit::Sdk::PlayerController pc(slot);
            if (!pc.IsValid() || !pc.IsAlive())
                return;
            pc.SetRender(RenderModeTransTexture, Palette[*idx]);
            *idx = (*idx + 1) % Palette.size();
        });

        // Auto-cancel after the duration; this routes through EffectManager so the cancel fn runs.
        Kit().Scheduler.Delay(DiscoDurationSec * 1000, [slot]() { Sys().Effects.Cancel(slot, EffectId::Disco); });

        auto cancel = [slot, savedMode, savedColor]() {
            CS2Kit::Sdk::PlayerController pc(slot);
            if (pc.IsValid())
                pc.SetRender(savedMode == 0 ? RenderModeNormal : savedMode,
                             savedColor == 0 ? ColorOpaqueWhite : savedColor);
        };

        return {timer, std::move(cancel), /*roundScoped*/ true};
    });

    Broadcast(ctx, on ? "broadcast.discoOn" : "broadcast.discoOff");
}

}  // namespace AdminSystem::Admin::Effects
