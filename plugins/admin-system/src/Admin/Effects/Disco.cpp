#include "Descriptors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <array>
#include <cstdint>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;

static constexpr auto RenderModeTransTexture = VoltMod::RenderMode_t::TransTexture;
static constexpr uint32_t ColorOpaqueWhite = 0xFFFFFFFFu;

// Bright RGBA values cycled at 200 ms - red / orange / yellow / green / blue / magenta.
static constexpr std::array<uint32_t, 6> Palette = {
    0xFF0000FFu, 0xFF8000FFu, 0xFFFF00FFu, 0x00FF00FFu, 0x0000FFFFu, 0xFF00FFFFu,
};

static constexpr int DiscoIntervalMs = 200;
static constexpr int DiscoDurationSec = 15;

Effect MakeDisco(VoltMod::Runtime& runtime)
{
    return Effect{.Permission = Flag(Permission::Fun),
                  .Id = static_cast<int>(EffectId::Disco),
                  .NameKey = "action.disco",
                  .OnKey = "broadcast.discoOn",
                  .OffKey = "broadcast.discoOff",
                  .Scope = EffectScope::Round,
                  .TickIntervalMs = DiscoIntervalMs,
                  .DurationMs = DiscoDurationSec * 1000,
                  .Setup = [&runtime](const ActionContext& ctx, int) -> EffectInstance {
                      auto savedMode = static_cast<VoltMod::RenderMode_t>(ctx.TargetPawn().RenderMode.Get());
                      uint32_t savedColor = ctx.TargetPawn().RenderColor;
                      int slot = ctx.Target().Slot();

                      return {.OnTick =
                                  [&entities = runtime.Entities, slot, idx = size_t{0}]() mutable {
                                      VoltMod::Pawn pawn = entities.PawnOf(slot);
                                      if (!pawn || !pawn.IsAlive())
                                          return;
                                      pawn.SetRender(RenderModeTransTexture, Palette[idx]);
                                      idx = (idx + 1) % Palette.size();
                                  },
                              .OnStop =
                                  [&entities = runtime.Entities, slot, savedMode, savedColor]() {
                                      VoltMod::Pawn pawn = entities.PawnOf(slot);
                                      if (pawn)
                                          pawn.SetRender(savedMode, savedColor == 0 ? ColorOpaqueWhite : savedColor);
                                  }};
                  }};
}

}  // namespace AdminSystem::Admin::Effects
