#include "BhopManager.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Sdk/Entity/Entity.hpp>
#include <VoltMod/Sdk/Entity/PlayerController.hpp>
#include <algorithm>
#include <cmath>
#include <mathlib/vector.h>

using namespace VoltMod;
namespace Log = VoltMod::Log;

namespace Bhop
{

void BhopManager::Initialize()
{
    ApplySettings();
    RegisterConsoleCommands();

    auto& events = _rt.Events;
    _subs.push_back(
        events.Listen<Events::PlayerSpawn>([this](const Events::PlayerSpawn& e) { OnPlayerSpawn(e.Slot); }));
    _subs.push_back(events.Listen<Events::PlayerJump>([this](const Events::PlayerJump& e) { OnPlayerJump(e.Slot); }));
    // Gamemode cfg re-exec on map change can reset the convars; re-asserting is cheap.
    _subs.push_back(events.Listen<Events::RoundStart>([this](const Events::RoundStart&) {
        if (_mode == Mode::Enabled)
            _conVars.ApplyGlobal();
    }));

    _rt.MovementHook.Install();
    _subs.push_back(_rt.MovementHook.ListenPre([this](int slot) { OnRunCommandPre(slot); }));
    _subs.push_back(_rt.MovementHook.ListenPost([this](int slot) { OnRunCommandPost(slot); }));

    // Grants need a server-side hop because subtick movement ignores the scoped
    // sv_autobunnyhopping override. Run after simulation so landing state is available.
    _subs.push_back(_rt.Scheduler.EveryFrame([this] {
        if (_mode != Mode::Grants)
            return;
        for (int slot = 0; slot < Core::MaxPlayers; ++slot)
            if (_grantedSlots[slot])
                ForceAutoHop(slot);
    }));
}

void BhopManager::ApplySettings()
{
    const BhopSettings& settings = _config.Get().bhop;

    if (settings.mode == "grants")
        _mode = Mode::Grants;
    else if (settings.mode == "enabled")
        _mode = Mode::Enabled;
    else
    {
        Log::Warn("Unknown bhop.mode '{}'; falling back to 'enabled'.", settings.mode);
        _mode = Mode::Enabled;
    }

    _conVars.Build(settings);

    if (_mode == Mode::Enabled)
        _conVars.ApplyGlobal();  // sets and replicates the overrides server-wide

    Log::Info("Bhop mode: {} ({} convar overrides).", _mode == Mode::Enabled ? "enabled" : "grants", _conVars.Count());
}

void BhopManager::Grant(int64_t steamId, bool enabled)
{
    if (enabled)
        _granted.insert(steamId);
    else
        _granted.erase(steamId);

    Player* player = _rt.Players.GetPlayerBySteamId(steamId);
    if (!player)
        return;

    int slot = player->GetSlot();
    if (!Core::IsValidSlot(slot))
        return;

    _grantedSlots[slot] = enabled;

    if (_mode == Mode::Grants)
    {
        if (enabled)
            _conVars.ReplicateOverrides(slot);
        else
            _conVars.ReplicateServerValues(slot);
    }

    if (_config.Get().bhop.notifyPlayer)
    {
        const char* key = enabled ? "bhop.granted" : "bhop.revoked";
        _rt.Messages.Send(slot, _rt.Translations.Get(key, slot), MessageKind::Center);
    }
}

void BhopManager::ReloadSettings()
{
    _conVars.Reset();

    if (!_config.Load(VoltMod::AddonFile(AddonName, "configs/settings.jsonc")))
    {
        Log::Warn("bhop_reload: settings.jsonc failed to load; keeping previous values in memory.");
        return;
    }

    ApplySettings();

    // Granted clients predict with the old values until told otherwise.
    if (_mode == Mode::Grants)
        for (int slot = 0; slot < Core::MaxPlayers; ++slot)
            if (_grantedSlots[slot])
                _conVars.ReplicateOverrides(slot);

    Log::Info("bhop_reload: settings re-applied.");
}

void BhopManager::OnPlayerDisconnect(Player* player)
{
    if (!player)
        return;

    // Grants are session-only: a reconnect starts clean.
    _granted.erase(player->GetSteamID());

    int slot = player->GetSlot();
    if (Core::IsValidSlot(slot))
    {
        _grantedSlots[slot] = false;
        _lastJump[slot] = {};
    }
}

void BhopManager::OnRunCommandPre(int slot)
{
    if (_mode == Mode::Grants && Core::IsValidSlot(slot) && _grantedSlots[slot])
        _conVars.FlipRaw();
}

void BhopManager::OnRunCommandPost(int /*slot*/)
{
    _conVars.RestoreRaw();
}

bool BhopManager::IsActiveSlot(int slot) const
{
    if (!Core::IsValidSlot(slot))
        return false;
    return _mode == Mode::Enabled || _grantedSlots[slot];
}

void BhopManager::OnPlayerJump(int slot)
{
    const HopBoostSettings& boost = _config.Get().bhop.hopBoost;
    if (!boost.enabled || !IsActiveSlot(slot))
        return;

    auto now = std::chrono::steady_clock::now();
    auto last = _lastJump[slot];
    _lastJump[slot] = now;

    bool chained = last.time_since_epoch().count() != 0 && now - last <= std::chrono::milliseconds(boost.chainWindowMs);
    if (!chained)
        return;

    PlayerController controller = _rt.Entities.Controller(slot);
    if (!controller.IsValid())
        return;

    Vector velocity = controller.GetVelocity();
    float speed = std::hypot(velocity.x, velocity.y);
    if (speed < 1.0f)
        return;

    float scaled = std::min(speed * boost.factor, std::max(boost.maxSpeed, speed));
    velocity.x *= scaled / speed;
    velocity.y *= scaled / speed;
    controller.SetVelocity(velocity);
}

void BhopManager::ForceAutoHop(int slot)
{
    if (!(_rt.Entities.GetPlayerButtons(slot) & Sdk::IN_JUMP))
        return;

    PlayerController controller = _rt.Entities.Controller(slot);
    if (!controller.IsValid())
        return;

    uint32_t flags = controller.GetFlags();
    if (!(flags & Sdk::FL_ONGROUND))
        return;

    Vector velocity = controller.GetVelocity();
    if (velocity.z > 0.0f)
        return;  // already ascending: the engine (or last frame's hop) took this jump

    constexpr float DefaultJumpImpulse = 301.993378f;  // sqrt(2 * 800 * 57.0); engine default
    velocity.z = _rt.ConVars.GetFloat("sv_jump_impulse").value_or(DefaultJumpImpulse);
    controller.SetVelocity(velocity);
    // Leave the ground in the same frame: if the next movement command still sees FL_ONGROUND
    // it can re-ground and zero the vertical velocity for a tick - the "laggy jump" hitch.
    controller.SetFlags(flags & ~Sdk::FL_ONGROUND);

    // A forced hop never emits player_jump, so feed the boost chain by hand.
    OnPlayerJump(slot);
}

void BhopManager::OnPlayerSpawn(int slot)
{
    // Enabled mode replicates server-wide via ApplyGlobal, so only grants needs per-player work.
    if (_mode != Mode::Grants || !Core::IsValidSlot(slot))
        return;

    Player* player = _rt.Players.GetPlayerBySlot(slot);
    bool granted = player && _granted.contains(player->GetSteamID());
    _grantedSlots[slot] = granted;

    // The connect/map-change convar snapshot clobbered the client's override; re-send.
    if (granted)
        _conVars.ReplicateOverrides(slot);
}

}  // namespace Bhop
