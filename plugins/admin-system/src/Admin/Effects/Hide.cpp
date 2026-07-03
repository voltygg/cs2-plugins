#include "Descriptors.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Sdk/PawnOps.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Sdk/TransmitFilter.hpp>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;
using CS2Kit::Sdk::PlayerController;
using CS2Kit::Sdk::TeamSpectator;

// Hide moves the player to the spectator team and stops transmitting their
// controller, which removes their row from every other client's scoreboard.
// The name blanking stays as a fallback for when the transmit filter is inert
// (missing gamedata offset after a CS2 update). Tradeoff the operator accepts:
// when the admin was the last human on a playing team, CS2's bot manager
// unloads bots until a human rejoins. Toggle off restores team, name and
// scoreboard visibility.

const EffectToggle Hide{Flag(Permission::Hide), EffectId::Hide, "broadcast.hideOn", "broadcast.hideOff",
                        [](const ActionContext& ctx) -> EffectSetup {
                            int savedTeam = ctx.TargetCtrl.GetTeam();
                            std::string savedName = ctx.TargetCtrl.GetPlayerName();

                            ctx.TargetCtrl.SetPlayerName("");
                            ctx.TargetCtrl.ChangeTeam(TeamSpectator);

                            int slot = ctx.Target->GetSlot();
                            Engine().Transmit.SetControllerHidden(slot, true);
                            return {0,
                                    [slot, savedTeam, savedName]() {
                                        Engine().Transmit.SetControllerHidden(slot, false);
                                        PlayerController pc(slot);
                                        if (!pc.IsValid())
                                            return;
                                        pc.SetPlayerName(savedName);
                                        if (pc.GetTeam() != savedTeam)
                                            pc.ChangeTeam(savedTeam);
                                    },
                                    false};
                        }};

}  // namespace AdminSystem::Admin::Effects
