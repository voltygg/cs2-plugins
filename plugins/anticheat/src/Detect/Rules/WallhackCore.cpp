#include "Detect/Rules/WallhackCore.hpp"

#include "Detect/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <utility>

namespace Anticheat
{

static constexpr float MinimumDistance = 150.0f;
/** Through a wall the aim only has to stay near the enemy, not on it. */
static constexpr float ToleranceMinDeg = 3.0f;
static constexpr float ToleranceHullFactor = 2.0f;
static constexpr int TrackingTicks = 64;
static constexpr int MaxOffRun = 6;
/** The enemy must have crossed enough of the view that a resting crosshair would have lost it. */
static constexpr float MinBearingTravelDeg = 6.0f;
static constexpr float FollowShare = 0.6f;
static constexpr int QuietTicks = 64;
static constexpr int PeekMinTicks = 24;
static constexpr int PeekWindowTicks = 16;
/** Walking is silent; anything faster is heard through walls. */
static constexpr float WalkSpeed = 135.0f;
static constexpr int SpeedWindowTicks = 16;
static constexpr int VictimFireMemoryTicks = 192;
static constexpr int TrackPoints = 2;
static constexpr int PeekPoints = 2;
static constexpr int WallbangPoints = 2;
static constexpr int HeadshotBonus = 1;
static constexpr int DetectionScore = 6;

/** 90% of the episode's samples must have stayed within tolerance. */
static constexpr bool MeetsCoverage(int onTarget, int samples)
{
    return Geometry::MeetsCoverage(onTarget, samples, 90);
}

void WallhackCore::Reset()
{
    _slots = {};
    _incidents = {};
}

void WallhackCore::OnSlotChanged(int slot)
{
    if (!InSlotRange(slot))
        return;
    _slots[slot] = {};
    _incidents[slot].Clear();
    for (auto& data : _slots)
    {
        if (data.Current.Target == slot)
            data.Current = {};
        if (data.PeekTarget == slot)
            data.PeekTarget = -1;
    }
}

void WallhackCore::OnSimulated(int slot, int32_t serverTick, const AimAngles& angles, const Vec3& eyePos)
{
    if (!InSlotRange(slot))
        return;
    _slots[slot].Pending = {.ServerTick = serverTick,
                            .Angles = angles,
                            .EyePos = eyePos,
                            .Valid = Geometry::IsFinite(angles) && Geometry::IsFinite(eyePos)};
}

void WallhackCore::OnWeaponFire(int slot, int32_t fireTick)
{
    if (InSlotRange(slot))
        _slots[slot].LastFireTick = fireTick;
}

void WallhackCore::CloseTrack(SlotData& data, int32_t serverTick, bool becameVisible)
{
    Track& track = data.Current;
    if (track.Target < 0)
        return;
    if (becameVisible && track.Samples >= PeekMinTicks)
    {
        data.PeekTarget = track.Target;
        data.PeekTick = serverTick;
    }
    if (track.Qualified)
        data.QuietUntilTick = serverTick + QuietTicks;
    track = {};
}

std::optional<Finding> WallhackCore::Count(int slot, int points, std::string evidence, double nowSec)
{
    const int total = _incidents[slot].Add(nowSec, points);
    if (total < DetectionScore)
        return std::nullopt;
    _incidents[slot].Clear();
    return Finding{.Kind = DetectionKind::Wallhack,
                   .Evidence = std::format("{} The rolling score reached {}/{}.", evidence, total, DetectionScore)};
}

float WallhackCore::Speed(int slot, int32_t serverTick) const
{
    const auto now = _shots.FindPosition(serverTick, slot);
    const auto before = _shots.FindPosition(serverTick - SpeedWindowTicks, slot);
    // Unknown motion reads as loud motion: no evidence rather than a guess.
    if (!now || !before || now->Teleported || before->Teleported)
        return 1e9f;
    return (now->Origin - before->Origin).Length() * TickRate / static_cast<float>(SpeedWindowTicks);
}

std::optional<Finding> WallhackCore::OnFrame(int slot, int32_t serverTick, bool aliveHuman, const LagEstimate& lag,
                                             double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot))
        return out;

    auto& data = _slots[slot];
    const AimSample sample = data.Pending;
    data.Pending = {};
    if (data.PeekTarget >= 0 && serverTick - data.PeekTick > PeekWindowTicks)
        data.PeekTarget = -1;

    const PositionFrame* frame = _shots.FindFrame(serverTick);
    const PositionFrame* past = lag.Valid ? _shots.FindFrame(serverTick - lag.Ticks) : nullptr;
    const PositionSample* observer = frame ? &frame->Players[slot] : nullptr;
    if (!aliveHuman || !sample.Valid || sample.ServerTick != serverTick || !past || !observer || !observer->Trackable())
    {
        CloseTrack(data, serverTick, false);
        return out;
    }

    struct Reading
    {
        AimAngles Bearing;
        float Error = 180.0f;
        float Tolerance = 0.0f;
        bool Hidden = false;
        bool Visible = false;
    };
    const Vec3 forward = Geometry::AimForward(sample.Angles);
    const auto read = [&](int target) -> std::optional<Reading> {
        const PositionSample& current = frame->Players[target];
        const PositionSample& seen = past->Players[target];
        if (target == slot || !seen.Trackable() || !_shots.IsOpponent(observer->Team, current))
            return std::nullopt;
        const Vec3 chest{seen.Origin.X, seen.Origin.Y, seen.Origin.Z + Geometry::BodyHeights[1]};
        const float distance = (chest - sample.EyePos).Length();
        if (!std::isfinite(distance) || distance < MinimumDistance)
            return std::nullopt;
        Reading reading;
        reading.Bearing = Geometry::Bearing(sample.EyePos, chest);
        reading.Error = Geometry::NearestBodyAimErrorAlong(sample.EyePos, forward, seen.Origin);
        reading.Tolerance = std::max(ToleranceMinDeg, ToleranceHullFactor *
                                                          Geometry::AngularSizeDeg(Geometry::PlayerHalfWidth, distance));
        reading.Hidden = current.HiddenFrom(slot);
        reading.Visible = current.SightKnownTo(slot) && current.VisibleTo(slot);
        if (!std::isfinite(reading.Error) || !Geometry::IsFinite(reading.Bearing))
            return std::nullopt;
        return reading;
    };

    Track& track = data.Current;
    if (track.Target >= 0 && track.LastTick != serverTick - 1)
        CloseTrack(data, serverTick, false);

    if (track.Target < 0)
    {
        if (serverTick < data.QuietUntilTick)
            return out;
        int best = -1;
        Reading bestReading;
        for (int target = 0; target < MaxSlots; ++target)
        {
            // Acquisition only wants hidden targets, and the sight stamps make that a bitmask test.
            if (!frame->Players[target].HiddenFrom(slot))
                continue;
            const std::optional<Reading> reading = read(target);
            if (reading && reading->Error <= reading->Tolerance &&
                (best < 0 || reading->Error < bestReading.Error))
            {
                best = target;
                bestReading = *reading;
            }
        }
        if (best < 0)
            return out;
        track = {.Target = best,
                 .StartTick = serverTick,
                 .LastTick = serverTick,
                 .Samples = 1,
                 .OnSamples = 1,
                 .LastAim = sample.Angles,
                 .LastBearing = bestReading.Bearing};
        return out;
    }

    const std::optional<Reading> reading = read(track.Target);
    if (!reading || reading->Visible || !reading->Hidden)
    {
        CloseTrack(data, serverTick, reading && reading->Visible);
        return out;
    }

    ++track.Samples;
    if (reading->Error <= reading->Tolerance)
    {
        ++track.OnSamples;
        track.OffRun = 0;
    }
    else if (++track.OffRun > MaxOffRun)
    {
        CloseTrack(data, serverTick, false);
        return out;
    }
    track.AimYawTravel += Geometry::YawDelta(track.LastAim.Yaw, sample.Angles.Yaw);
    track.BearingYawTravel += Geometry::YawDelta(track.LastBearing.Yaw, reading->Bearing.Yaw);
    track.LastAim = sample.Angles;
    track.LastBearing = reading->Bearing;
    track.LastTick = serverTick;

    // Following, not waiting: the aim turned the way the hidden enemy went, and most of the way.
    const bool followed = std::abs(track.BearingYawTravel) >= MinBearingTravelDeg &&
                          (track.AimYawTravel > 0.0f) == (track.BearingYawTravel > 0.0f) &&
                          std::abs(track.AimYawTravel) >= FollowShare * std::abs(track.BearingYawTravel);
    if (track.Qualified || track.Samples < TrackingTicks || !MeetsCoverage(track.OnSamples, track.Samples) || !followed)
        return out;

    track.Qualified = true;
    return Count(slot, TrackPoints,
                 std::format("The aim followed a hidden enemy through cover for {} ticks, turning {:.1f} degrees "
                             "as the enemy's bearing moved {:.1f}.",
                             track.Samples, track.AimYawTravel, track.BearingYawTravel),
                 nowSec);
}

std::optional<Finding> WallhackCore::OnShot(int slot, const ShotView& shot, const WallhackShotContext& context,
                                            double nowSec)
{
    std::optional<Finding> out;
    const int victim = shot.VictimSlot;
    if (!InSlotRange(slot) || shot.Slot != slot || !shot.HurtSeen || !InSlotRange(victim) || victim == slot)
        return out;

    auto& data = _slots[slot];
    if (data.PeekTarget == victim && shot.FireTick >= data.PeekTick && shot.FireTick - data.PeekTick <= PeekWindowTicks)
    {
        data.PeekTarget = -1;
        out = Count(slot, PeekPoints,
                    std::format("A hit landed {} ticks after the aim had followed the same enemy through cover "
                                "into view.",
                                shot.FireTick - data.PeekTick),
                    nowSec);
        if (out)
            return out;
    }

    const auto target = _shots.FindPosition(shot.FireTick, victim);
    const auto shooter = _shots.FindPosition(shot.FireTick, slot);
    if (!target || !shooter || !target->HiddenFrom(slot) || target->Teleported || shooter->Teleported ||
        context.TeamSawVictim || (target->Origin - shooter->EyePos).Length() < MinimumDistance)
        return out;

    // A hidden enemy who fired or ran recently gave their position away legitimately.
    const int32_t victimFired = _slots[victim].LastFireTick;
    if ((victimFired >= 0 && shot.FireTick - victimFired <= VictimFireMemoryTicks) ||
        Speed(victim, shot.FireTick) > WalkSpeed)
        return out;

    const int points = WallbangPoints + (shot.Headshot ? HeadshotBonus : 0);
    return Count(slot, points,
                 std::format("A {}shot through cover hit an enemy nobody on the team could see, who was neither "
                             "shooting nor running.",
                             shot.Headshot ? "head" : ""),
                 nowSec);
}

int WallhackCore::Score(int slot, double nowSec) const
{
    return InSlotRange(slot) ? _incidents[slot].Value(nowSec) : 0;
}

bool WallhackCore::IsTracking(int slot) const
{
    return InSlotRange(slot) && _slots[slot].Current.Target >= 0;
}

}  // namespace Anticheat
