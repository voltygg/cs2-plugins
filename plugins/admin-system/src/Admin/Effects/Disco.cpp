#include "Descriptors.hpp"
#include "EffectRegistry.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/App/Services.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <array>
#include <cstdint>

using CS2Kit::App::Engine;

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

const Effect Disco{.Permission = Flag(Permission::Fun),
                   .Id = static_cast<int>(EffectId::Disco),
                   .NameKey = "action.disco",
                   .OnKey = "broadcast.discoOn",
                   .OffKey = "broadcast.discoOff",
                   .Scope = EffectScope::Round,
                   .TickIntervalMs = DiscoIntervalMs,
                   .DurationMs = DiscoDurationSec * 1000,
                   .Setup = [](const ActionContext& ctx) -> EffectInstance {
                       uint8_t savedMode = ctx.TargetCtrl.GetRenderMode();
                       uint32_t savedColor = ctx.TargetCtrl.GetRenderColor();
                       int slot = ctx.Target->GetSlot();

                       return {.OnTick =
                                   [slot, idx = size_t{0}]() mutable {
                                       CS2Kit::PlayerController pc(slot);
                                       if (!pc.IsValid() || !pc.IsAlive())
                                           return;
                                       pc.SetRender(RenderModeTransTexture, Palette[idx]);
                                       idx = (idx + 1) % Palette.size();
                                   },
                               .OnStop =
                                   [slot, savedMode, savedColor]() {
                                       CS2Kit::PlayerController pc(slot);
                                       if (pc.IsValid())
                                           pc.SetRender(savedMode == 0 ? RenderModeNormal : savedMode,
                                                        savedColor == 0 ? ColorOpaqueWhite : savedColor);
                                   }};
                   }};

static const bool _registered = CS2Kit::Registry<EffectEntry>::Add({.Order = 20, .Toggle = &Disco});

}  // namespace AdminSystem::Admin::Effects
