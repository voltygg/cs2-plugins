#include "Hide.hpp"

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

    auto& mgr = EffectManager::Instance();
    if (mgr.IsActive(adminSlot, EffectId::Hide))
    {
        mgr.Cancel(adminSlot, EffectId::Hide);
        Broadcast(ctx, "broadcastHideOff");
        return;
    }

    int savedTeam = ctx.AdminCtrl.GetTeam();
    std::string savedName = ctx.AdminCtrl.GetPlayerName();

    ctx.AdminCtrl.SetPlayerName("");
    ctx.AdminCtrl.ChangeTeam(TeamSpectator);

    int slot = adminSlot;
    auto cancel = [slot, savedTeam, savedName]() {
        PlayerController pc(slot);
        if (!pc.IsValid())
            return;
        pc.SetPlayerName(savedName);
        if (pc.GetTeam() != savedTeam)
            pc.ChangeTeam(savedTeam);
    };

    mgr.Apply(adminSlot, EffectId::Hide, /*timerHandle*/ 0, std::move(cancel), /*roundScoped*/ false);
    Broadcast(ctx, "broadcastHideOn");
}

}  // namespace AdminSystem::Admin::Effects
