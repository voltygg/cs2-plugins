#include "Descriptors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Hooks/Transmit.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;

// Ghost drops the pawn (and its weapons/wearables) from network transmission to
// every other client instead of touching render fields, so the weapon, gloves and
// shadow vanish too. The player still sees themself; sounds are unaffected.

Effect MakeGhost(VoltMod::Runtime& runtime)
{
    return Effect{.Permission = Flag(Permission::Fun),
                  .Id = static_cast<int>(EffectId::Ghost),
                  .NameKey = "action.ghost",
                  .OnKey = "broadcast.ghostOn",
                  .OffKey = "broadcast.ghostOff",
                  .Setup = [&runtime](const ActionContext& ctx, int) -> EffectInstance {
                      int slot = ctx.Target().Slot();
                      auto& transmit = runtime.Transmit;
                      transmit.SetPawnHidden(slot, true);
                      return {.OnStop = [&transmit, slot]() { transmit.SetPawnHidden(slot, false); }};
                  }};
}

}  // namespace AdminSystem::Admin::Effects
