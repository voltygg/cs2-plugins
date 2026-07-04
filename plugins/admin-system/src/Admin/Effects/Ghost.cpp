#include "Descriptors.hpp"
#include "EffectRegistry.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Sdk/TransmitFilter.hpp>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;

// Ghost drops the pawn (and its weapons/wearables) from network transmission to
// every other client instead of touching render fields, so the weapon, gloves and
// shadow vanish too. The player still sees themself; sounds are unaffected.

const Effect Ghost{.Flag = Flag(Permission::Fun),
                   .Id = static_cast<int>(EffectId::Ghost),
                   .NameKey = "action.ghost",
                   .OnKey = "broadcast.ghostOn",
                   .OffKey = "broadcast.ghostOff",
                   .Setup = [](const ActionContext& ctx) -> EffectInstance {
                       int slot = ctx.Target->GetSlot();
                       Engine().Transmit.SetPawnHidden(slot, true);
                       return {.OnStop = [slot]() { Engine().Transmit.SetPawnHidden(slot, false); }};
                   }};

static const bool _registered = CS2Kit::Registry<EffectEntry>::Add({.Order = 10, .Toggle = &Ghost});

}  // namespace AdminSystem::Admin::Effects
