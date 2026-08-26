#include "FunMode.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>

namespace AdminSystem::Fun
{

namespace Log = VoltMod::Log;

namespace
{

constexpr const char* GravityCvar = "sv_gravity";
constexpr float LowGravityValue = 250.0f;
/** Only a last resort: the live `sv_gravity` is what gets restored, this is the engine stock
 *  value used when it cannot be read at all. */
constexpr float StockGravity = 800.0f;

constexpr const char* KnifeT = "weapon_knife_t";
constexpr const char* KnifeCT = "weapon_knife";

}  // namespace

FunMode::FunMode(VoltMod::Runtime& runtime) : _rt(runtime) {}

FunMode::~FunMode()
{
    // An unload must not leave the server at low gravity with nothing left to turn it off.
    RestoreGravity();
}

void FunMode::Start()
{
    namespace Events = VoltMod::Events;
    auto& events = _rt.Events;

    _subs.push_back(events.Listen<Events::RoundStart>([this](const Events::RoundStart&) { ApplyRoundStart(); }));

    // Knife rounds act per spawn, since a player who joins mid-round still has to get the
    // round's rules applied to them.
    _subs.push_back(events.Listen<Events::PlayerSpawn>([this](const Events::PlayerSpawn& e) {
        if (e.Slot >= 0 && _state.IsOn(Toggle::KnifeRound))
            GiveKnifeOnly(e.Slot);
    }));

    // The damage hook stays installed for the whole load cycle; the listener below returns on a
    // single bool-array read when no toggle is on, which is the normal state of a server.
    if (_rt.Damage.Install())
        _subs.push_back(_rt.Damage.ListenPreDamage([this](VoltMod::DamageView& view) { OnDamage(view); }));
    else
        Log::Warn("Fun Mode: the damage hook did not install; headshot-only, no-scope and one-hit-kill are inert.");
}

bool FunMode::Flip(Toggle toggle)
{
    bool on = _state.Flip(toggle);

    switch (toggle)
    {
    case Toggle::LowGravity:
        ApplyGravity();
        break;
    case Toggle::KnifeRound:
        // Acts on spawn; applying now covers everyone already alive.
        ApplyRoundStart();
        break;
    default:
        // The damage rules read the state directly, so there is nothing to apply.
        break;
    }
    return on;
}

void FunMode::ClearAll()
{
    _state.Clear();
    ApplyGravity();
}

void FunMode::RestoreGravity()
{
    if (!_savedGravity)
        return;
    SetGravity(*_savedGravity);
    _savedGravity.reset();
}

void FunMode::SetGravity(float value)
{
    // sv_gravity is FCVAR_REPLICATED, so the direct setters would not reach clients and their
    // prediction would disagree with the server; the console line is the one that networks.
    _rt.ConVars.ExecuteServerCommand(std::format("{} {}", GravityCvar, value).c_str());
}

void FunMode::ApplyGravity()
{
    if (_state.IsOn(Toggle::LowGravity))
    {
        // Snapshot on the way in so the server gets its own configured gravity back, not a
        // hardcoded 800. Taken per off->on transition rather than once at load, because an
        // operator can change sv_gravity in between - and re-asserted every round, since the
        // engine resets convars around a map change.
        if (!_savedGravity)
            _savedGravity = _rt.ConVars.GetFloat(GravityCvar).value_or(StockGravity);
        SetGravity(LowGravityValue);
        return;
    }

    // Restore only what this plugin took over; a server that never had low gravity on keeps
    // whatever its own cfg set.
    RestoreGravity();
}

void FunMode::ApplyRoundStart()
{
    ApplyGravity();

    if (!_state.IsOn(Toggle::KnifeRound))
        return;

    for (auto* player : _rt.Players.GetAllPlayers())
    {
        if (player)
            GiveKnifeOnly(player->GetSlot());
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

bool FunMode::IsScoped(int slot) const
{
    if (slot < 0)
        return false;
    auto controller = _rt.Entities.Controller(slot);
    return controller.IsValid() && controller.GetPawnField<bool>("CCSPlayerPawn", "m_bIsScoped");
}

void FunMode::OnDamage(VoltMod::DamageView& view)
{
    // Runs for every point of damage every player takes, so the all-off case must not touch the
    // engine at all - IsScoped alone costs a controller resolve plus two schema lookups.
    if (!_state.AnyOn())
        return;

    const bool scoped = _state.IsOn(Toggle::NoScopeOnly) && IsScoped(view.AttackerSlot);
    auto decision = DecideDamage(_state, view.Hitbox, scoped, view.AttackerSlot >= 0, view.Damage);
    view.Suppress = decision.Suppress;
    view.Damage = decision.Damage;
}

}  // namespace AdminSystem::Fun
