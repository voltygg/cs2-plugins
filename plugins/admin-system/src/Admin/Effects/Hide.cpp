#include "Descriptors.hpp"

#include <VoltMod/Runtime.hpp>
#include <VoltMod/Sdk/Entity/PawnOps.hpp>
#include <VoltMod/Sdk/Entity/PlayerController.hpp>
#include <VoltMod/Sdk/Visibility/GlowVision.hpp>
#include <VoltMod/Sdk/Visibility/TransmitFilter.hpp>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;
using VoltMod::Sdk::GlowVision;
using VoltMod::Sdk::PlayerController;
using VoltMod::Sdk::TeamSpectator;

// Hide moves the player to the spectator team and stops transmitting their
// controller, which removes their row from every other client's scoreboard.
// While hidden the admin also gets glow vision - every live player rendered as
// a team-colored glow through walls, transmit-filtered to the admin alone - so
// suspected wallhackers can be observed covertly. The toggle is deliberately
// silent (empty On/Off keys): a public broadcast would defeat the stealth.
// The name blanking stays as a fallback for when the transmit filter is inert
// (missing gamedata offset after a CS2 update). Tradeoff the operator accepts:
// when the admin was the last human on a playing team, CS2's bot manager
// unloads bots until a human rejoins. Toggle off restores team, name and
// scoreboard visibility.

const Effect Hide{.Permission = Flag(Permission::Hide),
                  .Id = static_cast<int>(EffectId::Hide),
                  .NameKey = "action.hide",
                  .OnKey = "",
                  .OffKey = "",
                  .TickIntervalMs = GlowVision::ReconcileIntervalMs,
                  .Setup = [](const ActionContext& ctx) -> EffectInstance {
                      int savedTeam = ctx.TargetCtrl.GetTeam();
                      std::string savedName = ctx.TargetCtrl.GetPlayerName();

                      ctx.TargetCtrl.SetPlayerName("");
                      ctx.TargetCtrl.ChangeTeam(TeamSpectator);

                      int slot = ctx.Target->GetSlot();
                      auto& transmit = ctx.Rt.Transmit;
                      transmit.SetControllerHidden(slot, true);

                      // Hide is persistent, so the reconcile tick rebuilds the glow clones after
                      // round restarts and tracks spawns/deaths/team changes across rounds.
                      auto glow = ctx.Rt.Visibility.CreateGlow(slot);
                      glow->Reconcile();

                      return {.OnTick = [glow]() { glow->Reconcile(); },
                              .OnStop =
                                  [&transmit, &entities = ctx.Rt.Entities, slot, savedTeam, savedName, glow]() {
                                      glow->Destroy();
                                      transmit.SetControllerHidden(slot, false);
                                      PlayerController pc = entities.Controller(slot);
                                      if (!pc.IsValid())
                                          return;
                                      pc.SetPlayerName(savedName);
                                      if (pc.GetTeam() != savedTeam)
                                          pc.ChangeTeam(savedTeam);
                                  }};
                  }};

}  // namespace AdminSystem::Admin::Effects
