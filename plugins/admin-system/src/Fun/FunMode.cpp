#include "FunMode.hpp"

#include "../Admin/Effects/Model.hpp"
#include "../Core/App.hpp"
#include "../Core/ChatService.hpp"

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

constexpr float LowGravityValue = 250.0f;
constexpr float DefaultGravityValue = 800.0f;

/** How often infinite money tops players back up. Matches CS2Fixes' infinite-ammo cadence. */
constexpr int64_t MoneyTopUpMs = 5000;
constexpr int MaxMoney = 16000;

constexpr const char* KnifeT = "weapon_knife_t";
constexpr const char* KnifeCT = "weapon_knife";

}  // namespace

FunMode::FunMode(App& app) : _app(app) {}

void FunMode::Start()
{
    namespace Events = VoltMod::Events;
    auto& events = _app.Runtime.Events;

    _subs.push_back(events.Listen<Events::RoundStart>([this](const Events::RoundStart&) { ApplyRoundStart(); }));

    // Knife rounds and chicken bots act per spawn, since a player who joins mid-round still has
    // to get the round's rules applied to them.
    _subs.push_back(events.Listen<Events::PlayerSpawn>([this](const Events::PlayerSpawn& e) {
        if (e.Slot < 0)
            return;
        if (_state.IsOn(Toggle::KnifeRound))
            GiveKnifeOnly(e.Slot);
        if (_state.IsOn(Toggle::ChickenBots))
            MakeChicken(e.Slot);
    }));

    // The damage hook stays installed for the whole load cycle; the listener itself is cheap and
    // returns immediately when no damage toggle is on.
    if (_app.Runtime.Damage.Install())
        _subs.push_back(_app.Runtime.Damage.ListenPreDamage([this](VoltMod::DamageView& view) { OnDamage(view); }));
    else
        Log::Warn("Fun Mode: the damage hook did not install; headshot-only, no-scope and one-hit-kill are inert.");

    _subs.push_back(_app.Runtime.Scheduler.Repeat(MoneyTopUpMs, [this] { TopUpMoney(); }));
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
    case Toggle::ChickenBots:
        // Both act on spawn; applying now covers everyone already alive.
        ApplyRoundStart();
        break;
    default:
        // The damage rules and the money top-up read the state directly, so nothing to apply.
        break;
    }
    return on;
}

void FunMode::ClearAll()
{
    _state.Clear();
    ApplyGravity();
}

void FunMode::ApplyGravity()
{
    // sv_gravity is FCVAR_REPLICATED, so the direct setters would not reach clients and their
    // prediction would disagree with the server; the console line is the one that networks.
    float value = _state.IsOn(Toggle::LowGravity) ? LowGravityValue : DefaultGravityValue;
    _app.Runtime.ConVars.ExecuteServerCommand(std::format("sv_gravity {}", value).c_str());
}

void FunMode::ApplyRoundStart()
{
    ApplyGravity();

    if (!_state.IsOn(Toggle::KnifeRound) && !_state.IsOn(Toggle::ChickenBots))
        return;

    for (auto* player : _app.Runtime.Players.GetAllPlayers())
    {
        if (!player)
            continue;
        int slot = player->GetSlot();
        if (_state.IsOn(Toggle::KnifeRound))
            GiveKnifeOnly(slot);
        if (_state.IsOn(Toggle::ChickenBots))
            MakeChicken(slot);
    }
}

void FunMode::GiveKnifeOnly(int slot)
{
    auto controller = _app.Runtime.Entities.Controller(slot);
    if (!controller.IsValid() || !controller.IsAlive())
        return;

    _app.Runtime.Items.StripWeapons(controller, false);
    _app.Runtime.Items.Give(controller, controller.GetTeam() == VoltMod::Sdk::TeamT ? KnifeT : KnifeCT);
}

void FunMode::MakeChicken(int slot)
{
    auto* player = _app.Runtime.Players.GetPlayerBySlot(slot);
    if (!player || !player->IsBot())
        return;

    auto controller = _app.Runtime.Entities.Controller(slot);
    if (!controller.IsValid() || !controller.IsAlive())
        return;

    // A real CChicken has no server-side spawn path worth trusting, so this is the chicken model
    // on the bot's own pawn - the same asset the fun-model effect already precaches.
    _app.Runtime.EntityOps.SetModel(controller.GetPawn(), Admin::Effects::ChickenModelPath);
}

void FunMode::TopUpMoney()
{
    if (!_state.IsOn(Toggle::InfiniteMoney))
        return;

    for (auto* player : _app.Runtime.Players.GetAllPlayers())
    {
        if (!player)
            continue;
        auto controller = _app.Runtime.Entities.Controller(player->GetSlot());
        if (controller.IsValid() && controller.GetMoney() < MaxMoney)
            controller.SetMoney(MaxMoney);
    }
}

bool FunMode::IsScoped(int slot) const
{
    if (slot < 0)
        return false;
    auto controller = _app.Runtime.Entities.Controller(slot);
    return controller.IsValid() && controller.GetPawnField<bool>("CCSPlayerPawn", "m_bIsScoped");
}

void FunMode::OnDamage(VoltMod::DamageView& view)
{
    auto decision = DecideDamage(_state, static_cast<int>(view.Hitbox), IsScoped(view.AttackerSlot),
                                 view.AttackerSlot >= 0, view.Damage);
    view.Suppress = decision.Suppress;
    view.Damage = decision.Damage;
}

}  // namespace AdminSystem::Fun
