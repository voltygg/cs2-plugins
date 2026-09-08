#include "Descriptors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Hooks/Visibility.hpp>
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
                      auto& visibility = runtime.Hooks.Visibility;
                      visibility.SetPawnHidden(slot, true);
                      return {.OnStop = [&visibility, slot]() { visibility.SetPawnHidden(slot, false); }};
                  }};
}

}  // namespace AdminSystem::Admin::Effects
