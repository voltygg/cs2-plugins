#include "../../Core/Managers.hpp"
#include "Descriptors.hpp"

#include <CS2Kit/Core/Scheduler.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <array>
#include <cstdint>
#include <memory>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;

namespace
{
constexpr uint8_t RenderModeNormal = 0;
constexpr uint8_t RenderModeTransTexture = 3;
constexpr uint32_t ColorOpaqueWhite = 0xFFFFFFFFu;

// Bright RGBA values cycled at 200 ms - red / orange / yellow / green / blue / magenta.
constexpr std::array<uint32_t, 6> Palette = {
    0xFF0000FFu, 0xFF8000FFu, 0xFFFF00FFu, 0x00FF00FFu, 0x0000FFFFu, 0xFF00FFFFu,
};

constexpr int DiscoIntervalMs = 200;
constexpr int DiscoDurationSec = 15;
}  // namespace

const EffectToggle Disco{Permission::Fun, EffectId::Disco, "broadcast.discoOn", "broadcast.discoOff",
                         [](const ActionContext& ctx) -> EffectSetup {
                             uint8_t savedMode = ctx.TargetCtrl.GetRenderMode();
                             uint32_t savedColor = ctx.TargetCtrl.GetRenderColor();
                             int slot = ctx.Target->GetSlot();

                             auto idx = std::make_shared<size_t>(0);
                             uint64_t timer = Engine().Scheduler.Repeat(DiscoIntervalMs, [slot, idx]() {
                                 CS2Kit::Sdk::PlayerController pc(slot);
                                 if (!pc.IsValid() || !pc.IsAlive())
                                     return;
                                 pc.SetRender(RenderModeTransTexture, Palette[*idx]);
                                 *idx = (*idx + 1) % Palette.size();
                             });

                             auto cancel = [slot, savedMode, savedColor]() {
                                 CS2Kit::Sdk::PlayerController pc(slot);
                                 if (pc.IsValid())
                                     pc.SetRender(savedMode == 0 ? RenderModeNormal : savedMode,
                                                  savedColor == 0 ? ColorOpaqueWhite : savedColor);
                             };

                             // EffectManager owns the auto-expire timer (DurationMs); a self-scheduled Delay here would
                             // survive an early cancel and could clobber a re-applied Disco on the same slot.
                             return {timer, std::move(cancel), /*roundScoped*/ true,
                                     /*durationMs*/ DiscoDurationSec * 1000};
                         }};

}  // namespace AdminSystem::Admin::Effects
