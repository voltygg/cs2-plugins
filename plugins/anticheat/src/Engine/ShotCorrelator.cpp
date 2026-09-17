#include "Engine/ShotCorrelator.hpp"

#include "Detect/Geometry.hpp"
#include "Detect/WeaponClass.hpp"
#include "Engine/CommandSample.hpp"
#include "Engine/VisualLag.hpp"

#include <VoltMod/Core/Slot.hpp>
#include <VoltMod/Core/Time.hpp>
#include <algorithm>
#include <cmath>
#include <eiface.h>
#include <mathlib/vector.h>
#include <optional>
#include <vector>

using VoltMod::IsValidSlot;

namespace Anticheat
{

using VoltMod::Time;

/** Origin and view angles jump discontinuously around a teleport, so the whole window is unreadable. */
static constexpr float TeleportGraceSec = 5.0f;
/** A shot older than this has received every event it can, so the cores may judge it. */
static constexpr int FinalizeAgeTicks = 2;
/** How far back a teammate's sight of the victim still counts as shared information. */
static constexpr int TeamSightMemoryTicks = 128;

void ShotCorrelator::Initialize()
{
    _userIds.fill(-1);

    auto& events = _rt.GameEvents;

    _subscriptions.Add(_rt.Hooks.Movement.Before +=
                       [this](int slot, const VoltMod::PlayerInput& cmd) { OnCommand(slot, cmd); });
    _subscriptions.Add(_rt.Scheduler.EveryFrame([this] { OnFrame(); }));

    // Subscribing arms the per-pawn hook, and the teleport grace window is ours, not the framework's.
    _lastTeleport.BindReset(_rt.Slots);
    _subscriptions.Add(_rt.Hooks.Teleport.Teleported += [this](int slot) {
        if (IsValidSlot(slot))
            _lastTeleport[slot] = _rt.Clock.Time();
    });

    _subscriptions.Add(events.On<VoltMod::PlayerSpawn>([this](const VoltMod::PlayerSpawn& e) {
        if (_detectors.ModuleEnabled(_detectors.AntiAim))
            _detectors.AntiAim.OnSlotChanged(e.Slot);
    }));
    _subscriptions.Add(events.On<VoltMod::WeaponFire>([this](const VoltMod::WeaponFire& e) { OnWeaponFire(e); }));
    _subscriptions.Add(events.On<VoltMod::BulletImpact>([this](const VoltMod::BulletImpact& e) { OnBulletImpact(e); }));
    // player_hurt carries the hitgroup SilentAim scores headshots from.
    _subscriptions.Add(events.On<VoltMod::PlayerHurt>([this](const VoltMod::PlayerHurt& e) { OnPlayerHurt(e); }));
    _subscriptions.Add(events.On<VoltMod::PlayerDeath>([this](const VoltMod::PlayerDeath& e) { OnPlayerDeath(e); }));
}

void ShotCorrelator::OnCommand(int slot, const VoltMod::PlayerInput& cmd)
{
    if (!cmd.Valid || !_detectors.Enabled() || !_detectors.IsEligible(slot))
        return;

    VoltMod::Pawn pawn = _rt.Entities.PawnOf(slot);
    if (!pawn)
        return;

    CmdSample sample = BuildSample(cmd);
    StampPawnState(sample, pawn);
    if (!Geometry::IsFinite(sample.EyePos))
        return;

    Detectors& d = _detectors;
    d.Correlator.OnCommand(slot, sample);
    if (d.ModuleEnabled(d.Aimbot))
        d.Aimbot.OnCommand(slot, sample);
    if (d.ModuleEnabled(d.AntiAim))
        d.AntiAim.OnCommand(slot, sample);
    if (d.ModuleEnabled(d.Recoil))
        d.Recoil.OnCommand(slot, sample);

    // This is the command the server is about to simulate, so ingest and stamp in the same pass.
    const auto serverTick = static_cast<int32_t>(_rt.Clock.Tick());
    const double now = Time::MonotonicSeconds();
    const bool teleported = JustTeleported(slot);
    d.Correlator.OnSimulated(slot, sample.CmdNum, serverTick, sample.EyePos, sample.Airborne);
    if (d.ModuleEnabled(d.Aimbot))
        d.Report(slot, d.Aimbot.OnSimulated(slot, sample.CmdNum, serverTick, sample.EyePos, now));
    if (d.ModuleEnabled(d.Aimlock))
        d.Aimlock.OnSimulated(slot, serverTick, sample.BaseAngles(), sample.EyePos);
    if (d.ModuleEnabled(d.Triggerbot))
        d.Triggerbot.OnSimulated(slot, serverTick, sample.BaseAngles(), sample.EyePos);
    if (d.ModuleEnabled(d.Wallhack))
        d.Wallhack.OnSimulated(slot, serverTick, sample.BaseAngles(), sample.EyePos);
    if (d.ModuleEnabled(d.Mouse))
        d.Report(slot, d.Mouse.OnSimulated(slot, sample, serverTick, teleported, now));
    if (d.ModuleEnabled(d.AntiAim))
        d.Report(slot, d.AntiAim.OnSimulated(slot, sample.CmdNum, serverTick, true, teleported, now));
}

bool ShotCorrelator::JustTeleported(int slot) const
{
    if (!IsValidSlot(slot))
        return false;

    const float stamp = _lastTeleport[slot];
    const float now = _rt.Clock.Time();
    // The clock restarts with the map, so a stamp ahead of it belongs to the previous one.
    return stamp != 0.0f && now >= stamp && now - stamp <= TeleportGraceSec;
}

void ShotCorrelator::CollectPositions(std::array<PositionSample, MaxSlots>& players,
                                      std::array<AimAngles, MaxSlots>& aims, std::array<bool, MaxSlots>& viewers)
{
    _userIds.fill(-1);
    IVEngineServer2* engine = _rt.Unsafe.Interfaces.Engine;
    _userIdsResolved = engine != nullptr;

    for (const VoltMod::Player* player : _rt.Players.All())
    {
        const int slot = player ? player->Slot() : -1;
        if (!IsValidSlot(slot))
            continue;
        if (engine)
            _userIds[slot] = engine->GetPlayerUserId(CPlayerSlot(slot)).Get();

        const VoltMod::Pawn pawn = _rt.Entities.PawnOf(slot);
        if (!pawn)
            continue;

        const Vector origin = pawn.Origin();
        const Vector eye = pawn.EyePosition();
        const QAngle angles = pawn.EyeAngles();
        players[slot] = {.Origin = {origin.x, origin.y, origin.z},
                         .EyePos = {eye.x, eye.y, eye.z},
                         .Team = pawn.Team(),
                         .Valid = true,
                         .Alive = pawn.IsAlive(),
                         .Teleported = JustTeleported(slot)};
        aims[slot] = {angles.x, angles.y};
        viewers[slot] = players[slot].Alive && _detectors.IsEligible(slot);
    }
}

void ShotCorrelator::OnFrame()
{
    Detectors& d = _detectors;
    if (!d.Enabled())
        return;

    const auto serverTick = static_cast<int32_t>(_rt.Clock.Tick());
    const double now = Time::MonotonicSeconds();

    // Two engine reads and a parse per estimate, so only measure it when a core will read it.
    const bool lagWanted =
        d.ModuleEnabled(d.Aimlock) || d.ModuleEnabled(d.Triggerbot) || d.ModuleEnabled(d.Wallhack);

    std::array<PositionSample, MaxSlots> players{};
    std::array<AimAngles, MaxSlots> aims{};
    std::array<bool, MaxSlots> viewers{};
    CollectPositions(players, aims, viewers);
    if (d.ModuleEnabled(d.Wallhack))
        _sight.Stamp(players, aims, viewers, d.Correlator);
    d.Correlator.CaptureFrame(serverTick, players);

    for (int slot = 0; slot < MaxSlots; ++slot)
    {
        const bool aliveHuman = viewers[slot];
        const bool eligible = aliveHuman || d.IsEligible(slot);
        const LagEstimate lag = aliveHuman && lagWanted ? MeasureVisualLag(_rt, slot) : LagEstimate{};

        if (d.ModuleEnabled(d.Aimbot))
            d.Report(slot, d.Aimbot.OnFrame(slot, serverTick, eligible, now));
        if (d.ModuleEnabled(d.Aimlock))
            d.Report(slot, d.Aimlock.OnFrame(slot, serverTick, aliveHuman, lag, now));
        if (d.ModuleEnabled(d.Triggerbot))
            d.Triggerbot.OnFrame(slot, serverTick, aliveHuman, lag);
        if (d.ModuleEnabled(d.Wallhack))
            d.Report(slot, d.Wallhack.OnFrame(slot, serverTick, aliveHuman, lag, now));
        if (d.ModuleEnabled(d.AntiAim))
            d.Report(slot, d.AntiAim.OnFrame(slot, serverTick, eligible, now));
        if (eligible && d.ModuleEnabled(d.Recoil))
            d.Report(slot, d.Recoil.OnFrame(slot, serverTick, now));
        if (eligible)
            FinalizeShots(slot, serverTick, now);
    }

    d.Correlator.Prune(serverTick);
}

bool ShotCorrelator::TeamSawVictim(int shooter, int victim, int32_t fireTick) const
{
    if (!IsValidSlot(shooter) || !IsValidSlot(victim))
        return false;

    const ShotCorrelatorCore& frames = _detectors.Correlator;
    const PositionFrame* frame = frames.FindFrame(fireTick);
    if (!frame || !frame->Players[shooter].Valid)
        return false;

    const int shooterTeam = frame->Players[shooter].Team;
    uint64_t teammates = 0;
    for (int mate = 0; mate < MaxSlots; ++mate)
    {
        const PositionSample& mateSample = frame->Players[mate];
        if (mate != shooter && mate != victim && mateSample.Alive && mateSample.Team == shooterTeam)
            teammates |= SlotBit(mate);
    }
    if (teammates == 0)
        return false;

    // Recent frames may already hold the answer from a teammate's own crosshair.
    uint64_t seenBy = 0;
    for (int32_t tick = fireTick; tick > fireTick - TeamSightMemoryTicks; --tick)
    {
        const PositionFrame* past = frames.FindFrame(tick);
        if (!past)
            continue;
        seenBy |= past->Players[victim].SeenBy;
        if ((seenBy & teammates) != 0)
            return true;
    }

    for (int mate = 0; mate < MaxSlots; ++mate)
        if ((teammates & SlotBit(mate)) != 0 && _sight.CanSee(mate, victim).value_or(false))
            return true;
    return false;
}

void ShotCorrelator::FinalizeShots(int slot, int32_t serverTick, double nowSec)
{
    Detectors& d = _detectors;
    const bool silentAim = d.ModuleEnabled(d.SilentAim);
    const bool wallhack = d.ModuleEnabled(d.Wallhack);

    // Reporting can kick, which clears the slot's shots - so report only after the walk.
    std::vector<Finding> findings;
    for (ShotView& shot : d.Correlator.Shots(slot))
    {
        if (shot.Finalized || static_cast<int64_t>(serverTick) - shot.FireTick < FinalizeAgeTicks)
            continue;
        shot.Finalized = true;

        if (silentAim)
            if (auto finding = d.SilentAim.Finalize(slot, shot, nowSec))
                findings.push_back(std::move(*finding));
        if (wallhack && shot.HurtSeen)
        {
            const WallhackShotContext context{.TeamSawVictim = TeamSawVictim(slot, shot.VictimSlot, shot.FireTick)};
            if (auto finding = d.Wallhack.OnShot(slot, shot, context, nowSec))
                findings.push_back(std::move(*finding));
        }
    }
    for (const Finding& finding : findings)
        d.Report(slot, finding);
}

void ShotCorrelator::OnWeaponFire(const VoltMod::WeaponFire& fire)
{
    Detectors& d = _detectors;
    if (!d.Enabled() || !d.IsEligible(fire.Slot) || !IsBallisticWeapon(fire.Weapon))
        return;

    const auto serverTick = static_cast<int32_t>(_rt.Clock.Tick());
    const double now = Time::MonotonicSeconds();
    if (d.ModuleEnabled(d.Triggerbot))
        d.Triggerbot.OnWeaponFire(fire.Slot, serverTick);
    if (d.ModuleEnabled(d.Wallhack))
        d.Wallhack.OnWeaponFire(fire.Slot, serverTick);

    const VoltMod::Pawn pawn = _rt.Entities.PawnOf(fire.Slot);
    const QAngle eyeAngles = pawn.EyeAngles();
    const AimAngles visible{eyeAngles.x, eyeAngles.y};
    // Without a pawn the field reads fabricate perfectly finite-looking zeros.
    const bool hasVisible = static_cast<bool>(pawn) && Geometry::IsFinite(visible);
    const int32_t fireCmd = pawn ? pawn.LastWeaponFireCommand() : 0;
    const int shotsFired = pawn ? pawn.ShotsFired() : 0;

    ShotView* shot =
        d.Correlator.OnWeaponFire(fire.Slot, fire.Weapon, serverTick, visible, hasVisible, fireCmd, shotsFired);
    if (!shot)
        return;
    if (d.ModuleEnabled(d.AntiAim))
        d.Report(fire.Slot, d.AntiAim.OnWeaponFire(fire.Slot, *shot, now));
    if (d.ModuleEnabled(d.Recoil))
        d.Report(fire.Slot, d.Recoil.OnShot(fire.Slot, *shot, now));
}

void ShotCorrelator::OnBulletImpact(const VoltMod::BulletImpact& impact)
{
    Detectors& d = _detectors;
    if (!d.Enabled())
        return;

    const auto serverTick = static_cast<int32_t>(_rt.Clock.Tick());
    // The event's userid is truncated to a byte, so only a slot whose in-window shot is unique
    // counts. Without a userid table the engine's own best-effort decode is all there is.
    int slot = d.Correlator.ResolveImpactShooter(impact.TruncatedUserId, serverTick, _userIds);
    if (slot < 0 && !_userIdsResolved)
        slot = impact.Slot;
    if (!d.IsEligible(slot))
        return;

    ShotView* shot = d.Correlator.OnBulletImpact(slot, {impact.X, impact.Y, impact.Z}, serverTick);
    if (shot && d.ModuleEnabled(d.SilentAim))
        d.SilentAim.OnShotUpdated(slot, *shot);
}

void ShotCorrelator::OnPlayerHurt(const VoltMod::PlayerHurt& hurt)
{
    Detectors& d = _detectors;
    if (!d.Enabled())
        return;

    const int attacker = hurt.AttackerSlot;
    const int victim = hurt.VictimSlot;
    if (!d.IsEligible(attacker) || !IsValidSlot(victim))
        return;

    const bool headshot = hurt.Hitbox == VoltMod::HitGroup::Head;
    ShotView* shot = d.Correlator.OnPlayerHurt(attacker, victim, headshot, static_cast<int32_t>(_rt.Clock.Tick()));
    if (!shot)
        return;

    const double now = Time::MonotonicSeconds();
    if (d.ModuleEnabled(d.SilentAim))
        d.SilentAim.OnShotUpdated(attacker, *shot);
    // Judged now, while the crosshair runs still describe the tick the shot was fired on.
    if (d.ModuleEnabled(d.Triggerbot))
        d.Report(attacker, d.Triggerbot.OnPlayerHurt(attacker, *shot, now));
    if (d.ModuleEnabled(d.Aimbot))
        d.Report(attacker, d.Aimbot.OnPlayerHurt(attacker, victim, *shot, now));
}

void ShotCorrelator::OnPlayerDeath(const VoltMod::PlayerDeath& death)
{
    if (!_detectors.Enabled() || !_detectors.IsEligible(death.AttackerSlot))
        return;

    // Nothing consumes the death directly: it only lands the wallbang flag SilentAim reads when it
    // finalizes two ticks later.
    _detectors.Correlator.OnPlayerDeath(death.AttackerSlot, death.VictimSlot, death.Weapon, death.Penetrated > 0,
                                        static_cast<int32_t>(_rt.Clock.Tick()));
}

}  // namespace Anticheat
