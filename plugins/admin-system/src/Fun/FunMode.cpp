#include "FunMode.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Events/EventTypes.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <concepts>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>


namespace AdminSystem::Fun
{

static constexpr const char* KnifeT = "weapon_knife_t";
static constexpr const char* KnifeCT = "weapon_knife";

FunMode::FunMode(VoltMod::Runtime& runtime) : _rt(runtime), _overrides(runtime.ConVars) {}

// ConVarOverrides restores everything the toggles took over, so an unload cannot leave the server
// at low gravity or on lethal damage scales with nothing left to turn them off.
FunMode::~FunMode() = default;

void FunMode::Start()
{
    ResolveConVars();

    auto& events = _rt.GameEvents;

    _subs.push_back(events.On<VoltMod::RoundStart>([this](const VoltMod::RoundStart&) { ApplyRoundStart(); }));

    // Per spawn, so a player who joins mid-round still gets the round's rules.
    _subs.push_back(events.On<VoltMod::PlayerSpawn>([this](const VoltMod::PlayerSpawn& e) {
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

    for (auto* player : _rt.Players.All())
    {
        if (player)
            GiveKnifeOnly(player->Slot());
    }
}

void FunMode::ResolveConVars()
{
    for (const auto& row : ToggleConVars)
    {
        const std::string name(row.Name);

        // Try the numeric form first: only headshot-only is a switch, and asking for the wrong
        // type is refused rather than silently writing nothing.
        if (auto number = VoltMod::ConVar<float>::Find(_rt.ConVars, name))
            _handles.push_back({.Owner = row.Owner, .OnValue = row.OnValue, .Handle = std::move(*number)});
        else if (auto flag = VoltMod::ConVar<bool>::Find(_rt.ConVars, name))
            _handles.push_back({.Owner = row.Owner, .OnValue = row.OnValue, .Handle = std::move(*flag)});
        else
            VoltMod::Log::Warn("Fun mode: convar '{}' unusable ({}); the toggle that drives it is inert.", name,
                               flag.error().Detail);
    }
}

void FunMode::ApplyOverrides()
{
    for (auto& handle : _handles)
    {
        const bool on = _state.IsOn(handle.Owner);
        std::visit(
            [&](auto& convar) {
                if (!on)
                {
                    _overrides.Restore(convar);
                    return;
                }
                // The one bool row's on-value is written as a bool, not as a float that happens
                // to be 1.0 - typed handles refuse the wrong type rather than writing nothing.
                if constexpr (std::same_as<std::remove_cvref_t<decltype(convar)>, VoltMod::ConVar<bool>>)
                    _overrides.Set(convar, handle.OnValue != 0.0f);
                else
                    _overrides.Set(convar, handle.OnValue);
            },
            handle.Handle);
    }
}

void FunMode::GiveKnifeOnly(int slot)
{
    auto pawn = _rt.Entities.PawnOf(slot);
    if (!pawn || !pawn.IsAlive())
        return;

    _rt.World.Items.StripWeapons(pawn, false);
    _rt.World.Items.Give(pawn, pawn.Team == VoltMod::TeamT ? KnifeT : KnifeCT);
}

}  // namespace AdminSystem::Fun
