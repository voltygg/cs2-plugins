#include "Descriptors.hpp"

#include <CS2Kit/Sdk/PawnOps.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;
using CS2Kit::Sdk::PlayerController;
using CS2Kit::Sdk::TeamSpectator;

// Hide moves the player to the spectator team and blanks their scoreboard
// name. Tradeoff the operator accepts: when the admin was the last human
// on a playing team, CS2's bot manager unloads bots until a human rejoins.
// Toggle off restores the original team and name.

const EffectToggle Hide{Flag(Permission::Hide), EffectId::Hide, "broadcast.hideOn", "broadcast.hideOff",
                        [](const ActionContext& ctx) -> EffectSetup {
                            int savedTeam = ctx.TargetCtrl.GetTeam();
                            std::string savedName = ctx.TargetCtrl.GetPlayerName();

                            ctx.TargetCtrl.SetPlayerName("");
                            ctx.TargetCtrl.ChangeTeam(TeamSpectator);

                            int slot = ctx.Target->GetSlot();
                            return {0,
                                    [slot, savedTeam, savedName]() {
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
