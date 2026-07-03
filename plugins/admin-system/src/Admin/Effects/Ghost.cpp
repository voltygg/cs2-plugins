#include "Descriptors.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Sdk/TransmitFilter.hpp>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;

// Ghost drops the pawn (and its weapons/wearables) from network transmission to
// every other client instead of touching render fields, so the weapon, gloves and
// shadow vanish too. The player still sees themself; sounds are unaffected.

const EffectToggle Ghost{Flag(Permission::Fun), EffectId::Ghost, "broadcast.ghostOn", "broadcast.ghostOff",
                         [](const ActionContext& ctx) -> EffectSetup {
                             int slot = ctx.Target->GetSlot();
                             Engine().Transmit.SetPawnHidden(slot, true);
                             return {0, [slot]() { Engine().Transmit.SetPawnHidden(slot, false); }, false};
                         }};

}  // namespace AdminSystem::Admin::Effects
