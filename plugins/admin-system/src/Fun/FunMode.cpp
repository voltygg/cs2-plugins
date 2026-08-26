#include "FunMode.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Fun
{

namespace
{

constexpr const char* KnifeT = "weapon_knife_t";
constexpr const char* KnifeCT = "weapon_knife";

}  // namespace

FunMode::FunMode(VoltMod::Runtime& runtime) : _rt(runtime), _conVars(runtime.ConVars) {}

// ~ConVarOverrides restores everything the toggles took over, so an unload cannot leave the server
// at low gravity or on lethal damage scales with nothing left to turn them off.
FunMode::~FunMode() = default;

void FunMode::Start()
{
    using namespace VoltMod;
    auto& events = _rt.Events;

    _subs.push_back(events.Listen<Events::RoundStart>([this](const Events::RoundStart&) { ApplyRoundStart(); }));

    // Per spawn, so a player who joins mid-round still gets the round's rules.
    _subs.push_back(events.Listen<Events::PlayerSpawn>([this](const Events::PlayerSpawn& e) {
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
            _conVars.Take(row.Name, row.OnValue, row.StockValue);
        else
            _conVars.Release(row.Name);
    }
}

void FunMode::GiveKnifeOnly(int slot)
{
    auto controller = _rt.Entities.Controller(slot);
    if (!controller.IsValid() || !controller.IsAlive())
        return;

    _rt.Items.StripWeapons(controller, false);
    _rt.Items.Give(controller, controller.GetTeam() == VoltMod::Sdk::TeamT ? KnifeT : KnifeCT);
}

}  // namespace AdminSystem::Fun
