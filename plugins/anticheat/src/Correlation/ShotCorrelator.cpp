#include "ShotCorrelator.hpp"

#include "AntiCheatManager.hpp"
#include "Core/Geometry.hpp"
#include "Detectors/AimlockDetector.hpp"

#include <VoltMod/Core/Slot.hpp>
#include <VoltMod/Core/Time.hpp>
#include <algorithm>
#include <cmath>
#include <eiface.h>
#include <mathlib/vector.h>
#include <optional>

using VoltMod::IsValidSlot;

namespace Anticheat
{

using VoltMod::Time;

/** Origin and view angles jump discontinuously around a teleport, so the whole window is unreadable. */
static constexpr float TeleportGraceSec = 5.0f;
/** A shot older than this has received every event it can, so SilentAim may score it. */
static constexpr int SilentFinalizeAgeTicks = 2;

static Vec3 ToVec3(const Vector& v)
{
    return {v.x, v.y, v.z};
}

static bool IsAirborne(const VoltMod::Pawn& pawn)
{
    const bool grounded = pawn.OnGroundLastTick || ((pawn.Flags.Get() & VoltMod::FL_ONGROUND) &&
                                                    pawn.GroundEntity.Get() != VoltMod::InvalidEntityHandle);
    return !grounded && pawn.Move() == VoltMod::MoveType::Walk &&
           pawn.ActualMoveTypeRaw.Get() == static_cast<uint8_t>(VoltMod::MoveType::Walk);
}

static CmdSample BuildSample(const VoltMod::UserCmdView& cmd)
{
    CmdSample sample;
    sample.CmdNum = cmd.CommandNumber;
    sample.ClientTick = cmd.ClientTick;
    sample.ViewPitch = cmd.ViewPitch;
    sample.ViewYaw = cmd.ViewYaw;
    sample.ViewRoll = cmd.ViewRoll;
    // A command that carried no viewangles leaves the fields at a perfectly ordinary-looking
    // (0,0,0), so the angles have to be untrusted rather than merely finite.
    sample.BaseAnglesFinite =
        cmd.HasViewAngles && Geometry::IsFinite(sample.BaseAngles()) && std::isfinite(sample.ViewRoll);

    for (int i = 0; i < cmd.SubtickMoveCount; ++i)
    {
        const VoltMod::SubtickMove& move = cmd.SubtickMoves[i];
        sample.SubtickPitchDelta += move.PitchDelta;
        sample.SubtickYawDelta += move.YawDelta;
        sample.SubtickAnglesFinite =
            sample.SubtickAnglesFinite && std::isfinite(move.PitchDelta) && std::isfinite(move.YawDelta);
    }

    const int attackIndex = cmd.Attack1StartHistoryIndex;
    sample.AttackStarted = attackIndex >= 0;
    // Only an index the client never sent is a fabrication. One the transport cap dropped is merely
    // absent, and must never be clamped back into range - that reads another shot's angles.
    sample.AttackIndexInvalid = attackIndex < -1 || attackIndex >= cmd.InputHistoryTotalCount;
    if (auto attack = cmd.SampleAt(attackIndex); attack && attack->HasViewAngles)
        sample.AttackAngles = AimAngles{attack->ViewPitch, attack->ViewYaw};

    for (int i = 0; i < cmd.InputHistorySampleCount; ++i)
    {
        const VoltMod::InputHistorySample& entry = cmd.InputHistorySamples[i];
        if (!entry.HasViewAngles)
            continue;
        sample.HasHistoryAngles = true;
        if (!std::isfinite(entry.ViewPitch) || !std::isfinite(entry.ViewYaw))
        {
            sample.HistoryAnglesFinite = false;
            continue;
        }
        sample.MaxHistoryYawDelta =
            std::max(sample.MaxHistoryYawDelta, std::abs(Geometry::YawDelta(sample.ViewYaw, entry.ViewYaw)));
    }
    return sample;
}

void ShotCorrelator::Initialize()
{
    _userIds.fill(-1);

    auto& events = _rt.GameEvents;

    _subscriptions.On(_rt.Hooks.Movement.PreCmd,
                      [this](int slot, const VoltMod::UserCmdView& cmd) { OnCommand(slot, cmd); });
    _subscriptions.Add(_rt.Scheduler.EveryFrame([this] { OnFrame(); }));

    // The aim modules discount the frames after a teleport, so the stamps live here rather than in
    // the framework: subscribing is what arms the per-pawn hook, and the grace window is ours.
    _lastTeleport.BindReset(_rt.Slots);
    _subscriptions.On(_rt.Hooks.Teleport.Teleported, [this](int slot) {
        if (IsValidSlot(slot))
            _lastTeleport[slot] = _rt.Clock.Time();
    });

    _subscriptions.Add(events.On<VoltMod::PlayerSpawn>([this](const VoltMod::PlayerSpawn& e) {
        if (_manager.ModuleEnabled(DetectionKind::AntiAim))
            _manager.AntiAim().OnSlotChanged(e.Slot);
    }));
    _subscriptions.Add(events.On<VoltMod::WeaponFire>([this](const VoltMod::WeaponFire& e) { OnWeaponFire(e); }));
    _subscriptions.Add(events.On<VoltMod::BulletImpact>([this](const VoltMod::BulletImpact& e) { OnBulletImpact(e); }));
    // player_hurt carries the hitgroup SilentAim scores headshots from.
    _subscriptions.Add(events.On<VoltMod::PlayerHurt>([this](const VoltMod::PlayerHurt& e) { OnPlayerHurt(e); }));
    _subscriptions.Add(events.On<VoltMod::PlayerDeath>([this](const VoltMod::PlayerDeath& e) { OnPlayerDeath(e); }));
}

void ShotCorrelator::OnCommand(int slot, const VoltMod::UserCmdView& cmd)
{
    if (!cmd.Valid || !_manager.DetectionsEnabled() || !_manager.IsEligible(slot))
        return;

    VoltMod::Pawn pawn = _rt.Entities.PawnOf(slot);
    if (!pawn)
        return;

    const Vec3 eye = ToVec3(pawn.EyePosition());
    if (!Geometry::IsFinite(eye))
        return;

    CmdSample sample = BuildSample(cmd);
    sample.EyePos = eye;
    sample.Airborne = IsAirborne(pawn);

    const bool aimbot = _manager.ModuleEnabled(DetectionKind::Aimbot);
    const bool aimlock = _manager.ModuleEnabled(DetectionKind::Aimlock);
    const bool antiAim = _manager.ModuleEnabled(DetectionKind::AntiAim);

    _manager.Correlator().OnCommand(slot, sample);
    if (aimbot)
        _manager.Aimbot().OnCommand(slot, sample);
    if (antiAim)
        _manager.AntiAim().OnCommand(slot, sample);

    // This is the command the server is about to simulate, so ingest and stamp in the same pass.
    const auto serverTick = static_cast<int32_t>(_rt.Clock.Tick());
    const double now = Time::MonotonicSeconds();
    _manager.Correlator().OnSimulated(slot, sample.CmdNum, serverTick, eye, sample.Airborne);
    if (aimbot)
        _manager.Report(slot, _manager.Aimbot().OnSimulated(slot, sample.CmdNum, serverTick, eye, now));
    if (aimlock)
        _manager.Aimlock().OnSimulated(slot, serverTick, sample.BaseAngles(), eye);
    if (antiAim)
        _manager.Report(
            slot, _manager.AntiAim().OnSimulated(slot, sample.CmdNum, serverTick, true, JustTeleported(slot), now));
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

void ShotCorrelator::CollectPositions(std::array<PositionSample, MaxSlots>& players)
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

        players[slot] = {.Origin = ToVec3(pawn.Origin()),
                         .EyePos = ToVec3(pawn.EyePosition()),
                         .Team = pawn.Team,
                         .Valid = true,
                         .Alive = pawn.IsAlive(),
                         .Teleported = JustTeleported(slot)};
    }
}

void ShotCorrelator::OnFrame()
{
    if (!_manager.DetectionsEnabled())
        return;

    const auto serverTick = static_cast<int32_t>(_rt.Clock.Tick());
    const double now = Time::MonotonicSeconds();

    std::array<PositionSample, MaxSlots> players{};
    CollectPositions(players);
    _manager.Correlator().CaptureFrame(serverTick, players);

    const bool aimbot = _manager.ModuleEnabled(DetectionKind::Aimbot);
    const bool aimlock = _manager.ModuleEnabled(DetectionKind::Aimlock);
    const bool antiAim = _manager.ModuleEnabled(DetectionKind::AntiAim);
    const bool silentAim = _manager.ModuleEnabled(DetectionKind::SilentAim);

    for (int slot = 0; slot < MaxSlots; ++slot)
    {
        const bool eligible = _manager.IsEligible(slot);
        if (aimbot)
            _manager.Report(slot, _manager.Aimbot().OnFrame(slot, serverTick, eligible, now));
        if (aimlock)
        {
            // Two engine reads and a parse per call, so only for the slots the estimate is used on.
            const bool aliveHuman = eligible && players[slot].Alive;
            _manager.Report(slot,
                            _manager.Aimlock().OnFrame(slot, serverTick, aliveHuman,
                                                       aliveHuman ? MeasureVisualLag(_rt, slot) : LagEstimate{}, now));
        }
        if (antiAim)
            _manager.Report(slot, _manager.AntiAim().OnFrame(slot, serverTick, eligible, now));
        if (silentAim && eligible)
            FinalizeSilentAim(slot, serverTick, now);
    }

    _manager.Correlator().Prune(serverTick);
}

void ShotCorrelator::FinalizeSilentAim(int slot, int32_t serverTick, double nowSec)
{
    // Reporting can kick, which clears the slot's shots - so report only after the walk.
    std::optional<Finding> finding;
    for (ShotView& shot : _manager.Correlator().Shots(slot))
    {
        if (shot.SilentConsumed || static_cast<int64_t>(serverTick) - shot.FireTick < SilentFinalizeAgeTicks)
            continue;
        finding = _manager.SilentAim().Finalize(slot, shot, nowSec);
        if (finding)
            break;
    }
    _manager.Report(slot, finding);
}

void ShotCorrelator::OnWeaponFire(const VoltMod::WeaponFire& fire)
{
    if (!_manager.DetectionsEnabled() || !_manager.IsEligible(fire.Slot))
        return;

    const VoltMod::Pawn pawn = _rt.Entities.PawnOf(fire.Slot);
    const QAngle eyeAngles = pawn.EyeAngles;
    const AimAngles visible{eyeAngles.x, eyeAngles.y};
    // Without a pawn the field read fabricates a perfectly finite-looking (0,0).
    const bool hasVisible = static_cast<bool>(pawn) && Geometry::IsFinite(visible);

    ShotView* shot = _manager.Correlator().OnWeaponFire(fire.Slot, fire.Weapon, static_cast<int32_t>(_rt.Clock.Tick()),
                                                        visible, hasVisible);
    if (shot && _manager.ModuleEnabled(DetectionKind::AntiAim))
        _manager.Report(fire.Slot, _manager.AntiAim().OnWeaponFire(fire.Slot, *shot, Time::MonotonicSeconds()));
}

void ShotCorrelator::OnBulletImpact(const VoltMod::BulletImpact& impact)
{
    if (!_manager.DetectionsEnabled())
        return;

    const auto serverTick = static_cast<int32_t>(_rt.Clock.Tick());
    // The event's userid is truncated to a byte, so only a slot whose in-window shot is unique
    // counts. Without a userid table the engine's own best-effort decode is all there is.
    int slot = _manager.Correlator().ResolveImpactShooter(impact.TruncatedUserId, serverTick, _userIds);
    if (slot < 0 && !_userIdsResolved)
        slot = impact.Slot;
    if (!_manager.IsEligible(slot))
        return;

    ShotView* shot = _manager.Correlator().OnBulletImpact(slot, {impact.X, impact.Y, impact.Z}, serverTick);
    if (shot && _manager.ModuleEnabled(DetectionKind::SilentAim))
        _manager.SilentAim().OnShotUpdated(slot, *shot);
}

void ShotCorrelator::OnPlayerHurt(const VoltMod::PlayerHurt& hurt)
{
    if (!_manager.DetectionsEnabled())
        return;

    const int attacker = hurt.AttackerSlot;
    const int victim = hurt.VictimSlot;
    if (!_manager.IsEligible(attacker) || !IsValidSlot(victim))
        return;

    const bool headshot = hurt.Hitbox == VoltMod::HitGroup::Head;
    ShotView* shot =
        _manager.Correlator().OnPlayerHurt(attacker, victim, headshot, static_cast<int32_t>(_rt.Clock.Tick()));
    if (!shot)
        return;

    if (_manager.ModuleEnabled(DetectionKind::SilentAim))
        _manager.SilentAim().OnShotUpdated(attacker, *shot);
    if (_manager.ModuleEnabled(DetectionKind::Aimbot))
        _manager.Report(attacker, _manager.Aimbot().OnPlayerHurt(attacker, victim, *shot, Time::MonotonicSeconds()));
}

void ShotCorrelator::OnPlayerDeath(const VoltMod::PlayerDeath& death)
{
    if (!_manager.DetectionsEnabled() || !_manager.IsEligible(death.AttackerSlot))
        return;

    // Nothing consumes the death directly: it only lands the wallbang flag SilentAim reads when it
    // finalizes two ticks later.
    _manager.Correlator().OnPlayerDeath(death.AttackerSlot, death.VictimSlot, death.Weapon, death.Penetrated > 0,
                                        static_cast<int32_t>(_rt.Clock.Tick()));
}

}  // namespace Anticheat
