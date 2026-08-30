#include "BhopManager.hpp"

#include <VoltMod/Core/EnumNames.hpp>
#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Entities/EntitySystem.hpp>
#include <VoltMod/Events/EventTypes.hpp>
#include <algorithm>
#include <cmath>
#include <mathlib/vector.h>
#include <string_view>
#include <utility>

using VoltMod::MaxPlayers;
using VoltMod::MessageKind;
using VoltMod::Pawn;
using VoltMod::Player;

namespace Log = VoltMod::Log;

namespace Bhop
{

void BhopManager::Initialize()
{
    // Forced hops read this every frame.
    if (auto impulse = _rt.ConVars.Find<float>("sv_jump_impulse"))
        _jumpImpulse = std::move(*impulse);
    else
        Log::Warn("sv_jump_impulse unusable ({}); forced hops use the engine default.", impulse.error().Detail);

    ApplySettings();
    RegisterConsoleCommands();

    auto& events = _rt.GameEvents;
    _subs.Add(events.On<VoltMod::PlayerSpawn>([this](const VoltMod::PlayerSpawn& e) { OnPlayerSpawn(e.Slot); }));
    _subs.Add(events.On<VoltMod::PlayerJump>([this](const VoltMod::PlayerJump& e) { OnPlayerJump(e.Slot); }));
    // Map changes can reset gamemode convars.
    _subs.Add(events.On<VoltMod::RoundStart>([this](const VoltMod::RoundStart&) {
        if (_mode == Mode::Enabled)
            _conVars.ApplyGlobal();
    }));

    _subs.On(_rt.Players.Disconnected, [this](VoltMod::Player& player) { OnPlayerDisconnect(player); });

    // Grants need a post-simulation hop because subtick movement ignores the scoped override.
    _subs.Add(_rt.Scheduler.EveryFrame([this] {
        if (_mode != Mode::Grants)
            return;
        for (int slot = 0; slot < MaxPlayers; ++slot)
            if (_grantedSlots[slot])
                ForceAutoHop(slot);
    }));
}

void BhopManager::ApplySettings()
{
    const BhopSettings& settings = _config.Get().bhop;

    if (auto parsed = VoltMod::Parse<Mode>(settings.mode))
        _mode = *parsed;
    else
    {
        Log::Warn("Unknown bhop.mode '{}'; falling back to 'enabled'.", settings.mode);
        _mode = Mode::Enabled;
    }

    _conVars.Build(settings);

    // Only grant mode needs per-player movement hooks.
    if (_mode == Mode::Grants && _movementSubs.Empty())
    {
        _movementSubs.On(_rt.Hooks.Movement.Pre, [this](int slot) { OnRunCommandPre(slot); });
        _movementSubs.On(_rt.Hooks.Movement.Post, [this](int slot) { OnRunCommandPost(slot); });
    }
    else if (_mode != Mode::Grants)
        _movementSubs.Clear();

    if (_mode == Mode::Enabled)
        _conVars.ApplyGlobal();

    Log::Info("Bhop mode: {} ({} convar overrides).", _mode == Mode::Enabled ? "enabled" : "grants", _conVars.Count());
}

void BhopManager::Grant(int64_t steamId, bool enabled)
{
    if (enabled)
        _granted.insert(steamId);
    else
        _granted.erase(steamId);

    Player* player = _rt.Players.BySteamId(steamId);
    if (!player)
        return;

    int slot = player->Slot();
    if (!VoltMod::IsValidSlot(slot))
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
        const std::string_view key = enabled ? "bhop.granted" : "bhop.revoked";
        _rt.Messages.Send(slot, _rt.Translations.Get(key, slot), MessageKind::Center);
    }
}

void BhopManager::ReloadSettings()
{
    _conVars.Reset();

    if (auto loaded = _config.Load(VoltMod::AddonFile(AddonName, "configs/settings.jsonc")); !loaded)
    {
        Log::Warn("bhop_reload: {}; keeping previous values in memory.", loaded.error().Detail);
        return;
    }

    ApplySettings();

    // Refresh prediction values for granted clients.
    if (_mode == Mode::Grants)
        for (int slot = 0; slot < MaxPlayers; ++slot)
            if (_grantedSlots[slot])
                _conVars.ReplicateOverrides(slot);

    Log::Info("bhop_reload: settings re-applied.");
}

void BhopManager::OnPlayerDisconnect(Player& player)
{
    // Grants do not survive reconnects.
    _granted.erase(player.SteamId());

    int slot = player.Slot();
    if (VoltMod::IsValidSlot(slot))
    {
        _grantedSlots[slot] = false;
        _lastJump[slot] = {};
    }
}

void BhopManager::OnRunCommandPre(int slot)
{
    if (_mode == Mode::Grants && VoltMod::IsValidSlot(slot) && _grantedSlots[slot])
        _conVars.HoldRaw();
}

void BhopManager::OnRunCommandPost(int /*slot*/)
{
    _conVars.ReleaseRaw();
}

bool BhopManager::IsActiveSlot(int slot) const
{
    if (!VoltMod::IsValidSlot(slot))
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

    Pawn pawn = _rt.Entities.PawnOf(slot);
    if (!pawn)
        return;

    Vector velocity = pawn.Velocity();
    float speed = std::hypot(velocity.x, velocity.y);
    if (speed < 1.0f)
        return;

    float scaled = std::min(speed * boost.factor, std::max(boost.maxSpeed, speed));
    velocity.x *= scaled / speed;
    velocity.y *= scaled / speed;
    pawn.SetVelocity(velocity);
}

void BhopManager::ForceAutoHop(int slot)
{
    if (!(_rt.Entities.Buttons(slot) & VoltMod::IN_JUMP))
        return;

    Pawn pawn = _rt.Entities.PawnOf(slot);
    if (!pawn)
        return;

    uint32_t flags = pawn.Flags();
    if (!(flags & VoltMod::FL_ONGROUND))
        return;

    Vector velocity = pawn.Velocity();
    if (velocity.z > 0.0f)
        return;  // The engine already applied the jump.

    constexpr float DefaultJumpImpulse = 301.993378f;  // sqrt(2 * 800 * 57.0)
    velocity.z = _jumpImpulse ? _jumpImpulse.Get() : DefaultJumpImpulse;
    pawn.SetVelocity(velocity);
    // Clear FL_ONGROUND now to prevent a one-tick re-grounding hitch.
    pawn.SetFlags(flags & ~VoltMod::FL_ONGROUND);

    // Forced hops do not emit player_jump.
    OnPlayerJump(slot);
}

void BhopManager::OnPlayerSpawn(int slot)
{
    // Enabled mode already replicates values server-wide.
    if (_mode != Mode::Grants || !VoltMod::IsValidSlot(slot))
        return;

    Player* player = _rt.Players.Get(slot);
    bool granted = player && _granted.contains(player->SteamId());
    _grantedSlots[slot] = granted;

    // Restore the override after the engine's spawn snapshot.
    if (granted)
        _conVars.ReplicateOverrides(slot);
}

}  // namespace Bhop
