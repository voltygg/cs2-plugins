#include "Hide.hpp"
#include "../../Core/Managers.hpp"

#include "../Actions/ActionContext.hpp"
#include "EffectManager.hpp"

#include <CS2Kit/Sdk/PlayerController.hpp>

namespace AdminSystem::Admin::Effects
{

using AdminSystem::Admin::Actions::Broadcast;
using AdminSystem::Admin::Actions::Resolve;
using CS2Kit::Sdk::PlayerController;

// Hide moves the admin to the spectator team and blanks their scoreboard
// name. Tradeoff the operator accepts: when the admin was the last human
// on a playing team, CS2's bot manager unloads bots until a human rejoins.
// Toggle off restores the original team and name.
namespace
{
constexpr int TeamSpectator = 1;
}  // namespace

void ToggleHide(int adminSlot)
{
    auto ctx = Resolve(adminSlot, adminSlot, 'b');
    if (!ctx.Valid())
        return;

    bool on = Sys().Effects.Toggle(adminSlot, EffectId::Hide, [&]() -> EffectSetup {
        int savedTeam = ctx.AdminCtrl.GetTeam();
        std::string savedName = ctx.AdminCtrl.GetPlayerName();

        ctx.AdminCtrl.SetPlayerName("");
        ctx.AdminCtrl.ChangeTeam(TeamSpectator);

        int slot = adminSlot;
        return {0, [slot, savedTeam, savedName]() {
                    PlayerController pc(slot);
                    if (!pc.IsValid())
                        return;
                    pc.SetPlayerName(savedName);
                    if (pc.GetTeam() != savedTeam)
                        pc.ChangeTeam(savedTeam);
                }, false};
    });

    Broadcast(ctx, on ? "broadcast.hideOn" : "broadcast.hideOff");
}

}  // namespace AdminSystem::Admin::Effects
