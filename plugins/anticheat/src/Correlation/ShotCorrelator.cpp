#include "ShotCorrelator.hpp"

#include "AntiCheatManager.hpp"
#include "Core/Geometry.hpp"
#include "Detectors/AimlockDetector.hpp"

#include <igameevents.h>

#include <CS2Kit/Core/Slot.hpp>
#include <algorithm>
#include <cmath>
#include <eiface.h>
#include <mathlib/vector.h>
#include <optional>

using CS2Kit::Core::Engine;
using CS2Kit::Core::IsValidSlot;

namespace Anticheat
{

namespace
{
/** Origin and view angles jump discontinuously around a teleport; nothing measured across one
 *  means anything, so the whole grace window is treated as unreadable. */
constexpr float TeleportGraceSec = 5.0f;
/** A shot older than this has received every event it can, so SilentAim may score it. */
constexpr int SilentFinalizeAgeTicks = 2;
constexpr uint32_t InvalidEntityHandle = 0xFFFFFFFFu;
constexpr int HitGroupGeneric = 0;
constexpr int HitGroupHead = 1;

Vec3 ToVec3(const Vector& v)
{
    return {v.x, v.y, v.z};
}

bool IsAirborne(const CS2Kit::PlayerController& controller)
{
    const uint32_t ground = controller.GetPawnField<uint32_t>("CBaseEntity", "m_hGroundEntity");
    const bool grounded = controller.GetPawnField<bool>("CCSPlayerPawn", "m_bOnGroundLastTick") ||
                          ((controller.GetFlags() & CS2Kit::Sdk::FL_ONGROUND) && ground != InvalidEntityHandle);
    const auto walk = static_cast<uint8_t>(CS2Kit::MoveType::Walk);
    return !grounded && controller.GetPawnField<uint8_t>("CBaseEntity", "m_MoveType") == walk &&
           controller.GetPawnField<uint8_t>("CBaseEntity", "m_nActualMoveType") == walk;
}

CmdSample BuildSample(const CS2Kit::UserCmdView& cmd)
{
    CmdSample sample;
    sample.CmdNum = cmd.CommandNumber;
    sample.ClientTick = cmd.ClientTick;
    sample.ViewPitch = cmd.ViewPitch;
    sample.ViewYaw = cmd.ViewYaw;
    sample.ViewRoll = cmd.ViewRoll;
    sample.ButtonsHeld = cmd.ButtonsHeld;
    sample.ButtonsChanged = cmd.ButtonsChanged;
    sample.MouseDx = cmd.MouseDx;
    sample.MouseDy = cmd.MouseDy;
    // A command that carried no viewangles leaves the view fields at their defaults, which read as
    // a perfectly ordinary (0,0,0) aim - so the angles are untrusted rather than merely finite.
    sample.BaseAnglesFinite =
        cmd.HasViewAngles && Geometry::IsFinite(sample.BaseAngles()) && std::isfinite(sample.ViewRoll);

    for (int i = 0; i < cmd.SubtickMoveCount; ++i)
    {
        const CS2Kit::SubtickMove& move = cmd.SubtickMoves[i];
        sample.SubtickPitchDelta += move.PitchDelta;
        sample.SubtickYawDelta += move.YawDelta;
        sample.SubtickAnglesFinite =
            sample.SubtickAnglesFinite && std::isfinite(move.PitchDelta) && std::isfinite(move.YawDelta);
    }

    const int attackIndex = cmd.Attack1StartHistoryIndex;
    sample.AttackStarted = attackIndex >= 0;
    // Only an index the client never sent is a fabrication; one the transport cap dropped is
    // merely absent, and must never be clamped back into range - that reads another shot's angles.
    sample.AttackIndexInvalid = attackIndex < -1 || attackIndex >= cmd.InputHistoryTotalCount;
    if (const CS2Kit::InputHistorySample* attack = cmd.SampleAt(attackIndex); attack && attack->HasViewAngles)
        sample.AttackAngles = AimAngles{attack->ViewPitch, attack->ViewYaw};

    for (int i = 0; i < cmd.InputHistorySampleCount; ++i)
    {
        const CS2Kit::InputHistorySample& entry = cmd.InputHistorySamples[i];
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
}  // namespace

void ShotCorrelator::Initialize()
{
    _userIds.fill(-1);

    Engine().MovementHook.ListenPreCmd([this](int slot, const CS2Kit::UserCmdView& cmd) { OnCommand(slot, cmd); });
    Engine().Scheduler.EveryFrame([this] { OnFrame(); });

    Engine().Events.Listen<CS2Kit::Events::PlayerSpawn>([this](const CS2Kit::Events::PlayerSpawn& e) {
        if (AntiCheatManager::ModuleEnabled(DetectionKind::AntiAim))
            _manager.AntiAim().OnSpawn(e.Slot);
    });
    Engine().Events.Listen<CS2Kit::Events::WeaponFire>(
        [this](const CS2Kit::Events::WeaponFire& e) { OnWeaponFire(e); });
    Engine().Events.Listen<CS2Kit::Events::BulletImpact>(
        [this](const CS2Kit::Events::BulletImpact& e) { OnBulletImpact(e); });
    // player_hurt carries the hitgroup SilentAim scores headshots from, which the typed view omits.
    Engine().Events.Listen("player_hurt", [this](IGameEvent* e) { OnPlayerHurt(e); });
    Engine().Events.Listen<CS2Kit::Events::PlayerDeath>(
        [this](const CS2Kit::Events::PlayerDeath& e) { OnPlayerDeath(e); });
}

void ShotCorrelator::OnCommand(int slot, const CS2Kit::UserCmdView& cmd)
{
    if (!cmd.Valid || !_manager.DetectionsEnabled() || !AntiCheatManager::IsEligible(slot))
        return;

    CS2Kit::PlayerController controller(slot);
    if (!controller.IsValid() || !controller.GetPawn())
        return;

    const Vec3 eye = ToVec3(controller.GetEyePosition());
    if (!Geometry::IsFinite(eye))
        return;

    CmdSample sample = BuildSample(cmd);
    sample.EyePos = eye;
    sample.Airborne = IsAirborne(controller);
    sample.Scoped = controller.GetPawnField<bool>("CCSPlayerPawn", "m_bIsScoped");

    const bool aimbot = AntiCheatManager::ModuleEnabled(DetectionKind::Aimbot);
    const bool aimlock = AntiCheatManager::ModuleEnabled(DetectionKind::Aimlock);
    const bool antiAim = AntiCheatManager::ModuleEnabled(DetectionKind::AntiAim);

    _manager.Correlator().OnCommand(slot, sample);
    if (aimbot)
        _manager.Aimbot().OnCommand(slot, sample);
    if (antiAim)
        _manager.AntiAim().OnCommand(slot, sample);

    // RunCommand is both CS2AC's usercmd ingest and its setup-move stamp: this is the command the
    // server is about to simulate, so the sample is ingested and stamped in the same pass.
    const auto serverTick = static_cast<int32_t>(CS2Kit::ServerTick());
    const double now = NowSeconds();
    _manager.Correlator().OnSimulated(slot, sample.CmdNum, serverTick, eye, sample.Airborne, sample.Scoped);
    if (aimbot)
        _manager.Report(slot, _manager.Aimbot().OnSimulated(slot, sample.CmdNum, serverTick, eye, now));
    if (aimlock)
        _manager.Aimlock().OnSimulated(slot, serverTick, sample.BaseAngles(), eye);
    if (antiAim)
        _manager.Report(slot,
                        _manager.AntiAim().OnSimulated(slot, sample.CmdNum, serverTick, true,
                                                       Engine().Teleports.JustTeleported(slot, TeleportGraceSec), now));
}

void ShotCorrelator::CollectPositions(std::array<PositionSample, MaxSlots>& players)
{
    _userIds.fill(-1);
    IVEngineServer2* engine = Engine().Interfaces.Engine;
    _userIdsResolved = engine != nullptr;

    for (const CS2Kit::Players::Player* player : Engine().Players.GetAllPlayers())
    {
        const int slot = player ? player->GetSlot() : -1;
        if (!IsValidSlot(slot))
            continue;
        if (engine)
            _userIds[slot] = engine->GetPlayerUserId(CPlayerSlot(slot)).Get();

        const CS2Kit::PlayerController controller(slot);
        if (!controller.IsValid() || !controller.GetPawn())
            continue;

        players[slot] = {.Origin = ToVec3(controller.GetAbsOrigin()),
                         .EyePos = ToVec3(controller.GetEyePosition()),
                         .Team = controller.GetTeam(),
                         .Valid = true,
                         .Alive = controller.IsAlive(),
                         .Teleported = Engine().Teleports.JustTeleported(slot, TeleportGraceSec)};
    }
}

void ShotCorrelator::OnFrame()
{
    if (!_manager.DetectionsEnabled())
        return;

    const auto serverTick = static_cast<int32_t>(CS2Kit::ServerTick());
    const double now = NowSeconds();

    std::array<PositionSample, MaxSlots> players{};
    CollectPositions(players);
    _manager.Correlator().CaptureFrame(serverTick, players);

    const bool aimbot = AntiCheatManager::ModuleEnabled(DetectionKind::Aimbot);
    const bool aimlock = AntiCheatManager::ModuleEnabled(DetectionKind::Aimlock);
    const bool antiAim = AntiCheatManager::ModuleEnabled(DetectionKind::AntiAim);
    const bool silentAim = AntiCheatManager::ModuleEnabled(DetectionKind::SilentAim);

    for (int slot = 0; slot < MaxSlots; ++slot)
    {
        const bool eligible = AntiCheatManager::IsEligible(slot);
        if (aimbot)
            _manager.Report(slot, _manager.Aimbot().OnFrame(slot, serverTick, eligible, now));
        if (aimlock)
            _manager.Report(slot, _manager.Aimlock().OnFrame(slot, serverTick, eligible && players[slot].Alive,
                                                             MeasureVisualLag(slot), now));
        if (antiAim)
            _manager.Report(slot, _manager.AntiAim().OnFrame(slot, serverTick, eligible, now));
        if (silentAim && eligible)
            FinalizeSilentAim(slot, serverTick, now);
    }

    _manager.Correlator().Prune(serverTick);
}

void ShotCorrelator::FinalizeSilentAim(int slot, int32_t serverTick, double nowSec)
{
    // Reporting can kick, which clears the slot's shots - so stop at the first verdict and report
    // it only once the deque is no longer being walked.
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

void ShotCorrelator::OnWeaponFire(const CS2Kit::Events::WeaponFire& fire)
{
    if (!_manager.DetectionsEnabled() || !AntiCheatManager::IsEligible(fire.Slot))
        return;

    const CS2Kit::PlayerController controller(fire.Slot);
    const QAngle eyeAngles = controller.GetEyeAngles();
    const AimAngles visible{eyeAngles.x, eyeAngles.y};
    // Without a pawn the field read yields a fabricated (0,0), which reads as a perfectly finite
    // aim; only a controller that has one can be trusted to have produced a real angle.
    const bool hasVisible = controller.IsValid() && controller.GetPawn() != nullptr && Geometry::IsFinite(visible);

    ShotView* shot = _manager.Correlator().OnWeaponFire(
        fire.Slot, fire.Weapon, static_cast<int32_t>(CS2Kit::ServerTick()), visible, hasVisible);
    if (shot && AntiCheatManager::ModuleEnabled(DetectionKind::AntiAim))
        _manager.Report(fire.Slot, _manager.AntiAim().OnWeaponFire(fire.Slot, *shot, NowSeconds()));
}

void ShotCorrelator::OnBulletImpact(const CS2Kit::Events::BulletImpact& impact)
{
    if (!_manager.DetectionsEnabled())
        return;

    const auto serverTick = static_cast<int32_t>(CS2Kit::ServerTick());
    // The event's userid is truncated to a byte, so identity alone cannot name the shooter: only a
    // slot whose in-window shot is unique counts. Without a userid table to match against, the
    // engine's own best-effort decode is all there is.
    int slot = _manager.Correlator().ResolveImpactShooter(impact.TruncatedUserId, serverTick, _userIds);
    if (slot < 0 && !_userIdsResolved)
        slot = impact.Slot;
    if (!AntiCheatManager::IsEligible(slot))
        return;

    ShotView* shot = _manager.Correlator().OnBulletImpact(slot, {impact.X, impact.Y, impact.Z}, serverTick);
    if (shot && AntiCheatManager::ModuleEnabled(DetectionKind::SilentAim))
        _manager.SilentAim().OnShotUpdated(slot, *shot);
}

void ShotCorrelator::OnPlayerHurt(IGameEvent* event)
{
    if (!event || !_manager.DetectionsEnabled())
        return;

    const int attacker = event->GetPlayerSlot("attacker").Get();
    const int victim = event->GetPlayerSlot("userid").Get();
    if (!AntiCheatManager::IsEligible(attacker) || !IsValidSlot(victim))
        return;

    const bool headshot = event->GetInt("hitgroup", HitGroupGeneric) == HitGroupHead;
    ShotView* shot =
        _manager.Correlator().OnPlayerHurt(attacker, victim, headshot, static_cast<int32_t>(CS2Kit::ServerTick()));
    if (!shot)
        return;

    if (AntiCheatManager::ModuleEnabled(DetectionKind::SilentAim))
        _manager.SilentAim().OnShotUpdated(attacker, *shot);
    if (AntiCheatManager::ModuleEnabled(DetectionKind::Aimbot))
        _manager.Report(attacker, _manager.Aimbot().OnPlayerHurt(attacker, victim, *shot, NowSeconds()));
}

void ShotCorrelator::OnPlayerDeath(const CS2Kit::Events::PlayerDeath& death)
{
    if (!_manager.DetectionsEnabled() || !AntiCheatManager::IsEligible(death.AttackerSlot))
        return;

    // Nothing consumes the death directly; it lands the wallbang flag on the shot, which SilentAim
    // reads when it finalizes two ticks later.
    _manager.Correlator().OnPlayerDeath(death.AttackerSlot, death.VictimSlot, death.Weapon, death.Penetrated > 0,
                                        static_cast<int32_t>(CS2Kit::ServerTick()));
}

}  // namespace Anticheat
