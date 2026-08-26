#include "FunMode.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>

using VoltMod::PlayerManager;
using VoltMod::Runtime;

namespace AdminSystem::Fun
{

static constexpr const char* KnifeT = "weapon_knife_t";
static constexpr const char* KnifeCT = "weapon_knife";

FunMode::FunMode(VoltMod::Runtime& runtime) : _rt(runtime), _lease(runtime.ConVars) {}

// ~ConVarLease restores everything the toggles took over, so an unload cannot leave the server
// at low gravity or on lethal damage scales with nothing left to turn them off.
FunMode::~FunMode() = default;

void FunMode::Start()
{
    auto& events = _rt.Events;

    _subs.push_back(events.Listen<VoltMod::RoundStart>([this](const VoltMod::RoundStart&) { ApplyRoundStart(); }));

    // Per spawn, so a player who joins mid-round still gets the round's rules.
    _subs.push_back(events.Listen<VoltMod::PlayerSpawn>([this](const VoltMod::PlayerSpawn& e) {
        if (e.Slot >= 0 && _state.IsOn(Toggle::KnifeRound))
            GiveKnifeOnly(e.Slot);
    }));
}

bool FunMode::Flip(Toggle toggle)
{
    const bool on = _state.Flip(toggle);

    if (toggle == Toggle::KnifeRound)
        ApplyRoundStart();  // acts on spawn; applying now covers everyone already alive
    else
        ApplyOverrides();

    return on;
}

void FunMode::ClearAll()
{
    _state.Clear();
    ApplyOverrides();
}

void FunMode::ApplyRoundStart()
{
    // Re-applied every round, not only when flipped: the engine resets convars around a map
    // change, which would otherwise silently drop the active toggles.
    ApplyOverrides();

    if (!_state.IsOn(Toggle::KnifeRound))
        return;

    for (auto* player : _rt.Players.GetAllPlayers())
    {
        if (player)
            GiveKnifeOnly(player->GetSlot());
    }
}

void FunMode::ApplyOverrides()
{
    for (const auto& row : ToggleConVars)
    {
        if (_state.IsOn(row.Owner))
            _lease.Override(row.Name, row.OnValue);
        else
            _lease.Restore(row.Name);
    }
}

void FunMode::GiveKnifeOnly(int slot)
{
    auto controller = _rt.Entities.Controller(slot);
    if (!controller.IsValid() || !controller.IsAlive())
        return;

    _rt.Items.StripWeapons(controller, false);
    _rt.Items.Give(controller, controller.GetTeam() == VoltMod::TeamT ? KnifeT : KnifeCT);
}

}  // namespace AdminSystem::Fun
