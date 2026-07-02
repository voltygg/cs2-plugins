#include "Descriptors.hpp"

#include <CS2Kit/Sdk/PlayerController.hpp>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;
using CS2Kit::Sdk::PlayerController;

const EffectToggle Ghost{Flag(Permission::Fun), EffectId::Ghost, "broadcast.ghostOn", "broadcast.ghostOff",
                         [](const ActionContext& ctx) -> EffectSetup {
                             ctx.TargetCtrl.SetVisible(false);
                             int slot = ctx.Target->GetSlot();
                             return {0,
                                     [slot]() {
                                         PlayerController pc(slot);
                                         if (pc.IsValid())
                                             pc.SetVisible(true);
                                     },
                                     false};
                         }};

}  // namespace AdminSystem::Admin::Effects
