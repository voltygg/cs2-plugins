#include "Admin/Effects/Descriptors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <array>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;
using VoltMod::Color;

static constexpr std::array<Color, 6> Palette = {
    Color{255, 0, 0}, Color{255, 128, 0}, Color{255, 255, 0}, Color{0, 255, 0}, Color{0, 0, 255}, Color{255, 0, 255},
};

static constexpr int DiscoIntervalMs = 200;
static constexpr int DiscoDurationSec = 15;

Effect MakeDisco(VoltMod::Runtime& runtime)
{
    return Effect{.Permission = Permission::Fun,
                  .Id = EffectId::Disco,
                  .NameKey = "action.disco",
                  .OnKey = "broadcast.discoOn",
                  .OffKey = "broadcast.discoOff",
                  .Scope = EffectScope::Round,
                  .TickIntervalMs = DiscoIntervalMs,
                  .DurationMs = DiscoDurationSec * 1000,
                  .Setup = [&runtime](const ActionContext& ctx, int) -> EffectInstance {
                      const VoltMod::Pawn target = ctx.Target().Pawn();
                      const auto savedMode = target.RenderMode();
                      // A pawn that never set a colour reads all zero, which would restore it invisible.
                      const Color savedColor =
                          target.RenderColor() == Color{0, 0, 0, 0} ? Color{} : target.RenderColor();
                      const int slot = ctx.Target().Slot();

                      return {.OnTick =
                                  [&entities = runtime.Entities, slot, index = size_t{0}]() mutable {
                                      const VoltMod::Pawn pawn = entities.Pawn(slot);
                                      if (!pawn.IsAlive())
                                      {
                                          return;
                                      }
                                      pawn.SetRender(VoltMod::Schema::RenderMode_t::kRenderTransAlpha, Palette[index]);
                                      index = (index + 1) % Palette.size();
                                  },
                              .OnStop = [&entities = runtime.Entities, slot, savedMode,
                                         savedColor]() { entities.Pawn(slot).SetRender(savedMode, savedColor); }};
                  }};
}

}  // namespace AdminSystem::Admin::Effects
