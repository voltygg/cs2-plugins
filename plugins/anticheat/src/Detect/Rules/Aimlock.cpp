#include "Detect/Rules/Aimlock.hpp"

#include "Detect/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace Anticheat::Rules
{

/** Three tracking episodes are what this rule reports on alone. */
static constexpr float PerEpisode = 1.0f / 3.0f;

static constexpr int TrackingTicks = static_cast<int>(TickRate * 1.5f);   // 96
static constexpr int OffTargetTicks = static_cast<int>(TickRate * 0.5f);  // 32
static constexpr float MinimumDistance = 200.0f;
static constexpr float MinimumTargetTravel = 48.0f;  // one and a half player widths, as degrees at that range

/** 95% of the episode's samples must have been inside the target's angular width. */
static constexpr bool MeetsCoverage(int onTarget, int samples)
{
    return Geometry::MeetsCoverage(onTarget, samples, 95);
}

struct TargetEvaluation
{
    AimAngles Bearing;
    float Error = 180.0f;
    float MaximumError = 0.0f;
    float RequiredDisplacement = 0.0f;
    bool Valid = false;

    bool OnTarget() const { return Valid && Error <= MaximumError; }
};

static TargetEvaluation EvaluateTarget(const ShotHistory& shots, const AimAngles& angles, const Vec3& eyePos,
                                       const PositionFrame& currentFrame, const PositionFrame* historical,
                                       int observerSlot, int targetSlot, int bodyPoint)
{
    TargetEvaluation result;
    if (!InSlotRange(observerSlot) || !InSlotRange(targetSlot) || bodyPoint < 0 ||
        bodyPoint >= Geometry::BodyPointCount || !Geometry::IsFinite(eyePos) || !Geometry::IsFinite(angles))
        return result;

    const PositionSample& observer = currentFrame.Players[observerSlot];
    const PositionSample& currentTarget = currentFrame.Players[targetSlot];
    if (!observer.Trackable() || !shots.IsOpponent(observer.Team, currentTarget) || !historical)
        return result;

    const PositionSample& target = historical->Players[targetSlot];
    if (!shots.IsOpponent(observer.Team, target) || (target.Origin - eyePos).Length() < MinimumDistance)
        return result;

    const Vec3 point = {target.Origin.X, target.Origin.Y, target.Origin.Z + Geometry::BodyHeights[bodyPoint]};
    const float distance = (point - eyePos).Length();
    if (!Geometry::IsFinite(point) || !std::isfinite(distance) || distance < 1e-3f)
        return result;

    result.Error = Geometry::AimErrorDeg(eyePos, angles, point);
    result.MaximumError = Geometry::AngularSizeDeg(Geometry::PlayerHalfWidth, distance);
    result.RequiredDisplacement = Geometry::AngularSizeDeg(MinimumTargetTravel, distance);
    result.Bearing = Geometry::Bearing(eyePos, point);
    result.Valid = std::isfinite(result.Error) && std::isfinite(result.MaximumError) &&
                   std::isfinite(result.RequiredDisplacement) && Geometry::IsFinite(result.Bearing);
    return result;
}

struct Candidate
{
    int TargetSlot = -1;
    int BodyPoint = -1;
    float Error = 180.0f;
    bool Valid = false;
};

/** The single target the aim is already inside; two candidates means no episode. Runs per alive
 *  player per frame, so it rejects the observer and each target once before the inner search. */
static Candidate FindCandidate(const ShotHistory& shots, const AimAngles& angles, const Vec3& eyePos,
                               const PositionFrame& frame, const LagFrames& lagFrames, int observerSlot)
{
    Candidate best;
    if (lagFrames.Last < 0 || !InSlotRange(observerSlot) || !Geometry::IsFinite(eyePos) || !Geometry::IsFinite(angles))
        return best;

    const PositionSample& observer = frame.Players[observerSlot];
    if (!observer.Trackable())
        return best;

    int matchedTarget = -1;
    bool ambiguous = false;
    for (int targetSlot = 0; targetSlot < MaxSlots; ++targetSlot)
    {
        if (targetSlot == observerSlot)
            continue;
        const PositionSample& currentTarget = frame.Players[targetSlot];
        if (!shots.IsOpponent(observer.Team, currentTarget))
            continue;

        for (int lagTicks = lagFrames.First; lagTicks <= lagFrames.Last; ++lagTicks)
        {
            const PositionFrame* historical = lagFrames.Frames[lagTicks - lagFrames.First];
            for (int bodyPoint = 0; bodyPoint < Geometry::BodyPointCount; ++bodyPoint)
            {
                const TargetEvaluation evaluation =
                    EvaluateTarget(shots, angles, eyePos, frame, historical, observerSlot, targetSlot, bodyPoint);
                if (!evaluation.OnTarget())
                    continue;
                if (matchedTarget < 0)
                    matchedTarget = targetSlot;
                else if (matchedTarget != targetSlot)
                    ambiguous = true;
                if (evaluation.Error < best.Error)
                    best = {targetSlot, bodyPoint, evaluation.Error, true};
            }
        }
    }
    best.Valid = best.Valid && !ambiguous;
    return best;
}

void Aimlock::Reset()
{
    _slots = {};
}

void Aimlock::ClearSlot(int slot)
{
    if (!InSlotRange(slot))
        return;
    _slots[slot] = {};
}

void Aimlock::OnSimulated(int slot, int32_t serverTick, const AimAngles& angles, const Vec3& eyePos)
{
    if (!InSlotRange(slot))
        return;

    auto& data = _slots[slot];
    if (serverTick == data.LastProcessedTick)
        return;
    if (!Geometry::IsFinite(angles) || !Geometry::IsFinite(eyePos))
    {
        data.Current = {};
        data.Pending = {};
        return;
    }
    data.Pending = {.ServerTick = serverTick, .Angles = angles, .EyePos = eyePos, .Valid = true};
}

void Aimlock::OnFrame(int slot, int32_t serverTick, bool aliveHuman, const ViewLag& lag, double nowSec)
{
    if (!InSlotRange(slot))
        return;

    auto& data = _slots[slot];
    if (!aliveHuman)
    {
        // Only the tracking state is unusable across a death - counted episodes stay in _incidents,
        // so dying between them cannot wipe the evidence.
        data = {};
        return;
    }
    if (!data.Pending.Valid)
        return;

    const Sample sample = data.Pending;
    data.Pending = {};
    // The simulated command and the world snapshot must describe one and the same new tick.
    if (sample.ServerTick != serverTick || sample.ServerTick == data.LastProcessedTick || !_shots.FindFrame(serverTick))
    {
        data.Current = {};
        return;
    }
    data.LastProcessedTick = sample.ServerTick;
    Evaluate(slot, data, sample, lag, nowSec);
}

/** Whether any lag hypothesis still puts the aim on @p targetSlot's @p bodyPoint. */
bool Aimlock::StillOnTarget(const Sample& sample, const PositionFrame& frame, const LagFrames& lagFrames, int slot,
                            int targetSlot, int bodyPoint) const
{
    for (int lagTicks = lagFrames.First; lagTicks <= lagFrames.Last; ++lagTicks)
    {
        const TargetEvaluation evaluation =
            EvaluateTarget(_shots, sample.Angles, sample.EyePos, frame, lagFrames.Frames[lagTicks - lagFrames.First],
                           slot, targetSlot, bodyPoint);
        if (evaluation.OnTarget())
            return true;
    }
    return false;
}

void Aimlock::Evaluate(int slot, SlotData& data, const Sample& sample, const ViewLag& lag, double nowSec)
{
    const PositionFrame* frame = _shots.FindFrame(sample.ServerTick);
    if (!frame)
    {
        data.Current = {};
        return;
    }
    const LagFrames lagFrames = ResolveLagFrames(_shots, sample.ServerTick, lag);

    if (data.Locked)
    {
        // After a detection, stay quiet until the player leaves the target for half a second.
        if (StillOnTarget(sample, *frame, lagFrames, slot, data.LockedTarget, data.LockedBodyPoint))
        {
            data.OffTargetSince = -1;
            return;
        }
        if (data.OffTargetSince < 0)
            data.OffTargetSince = sample.ServerTick;
        if (static_cast<int64_t>(sample.ServerTick) - data.OffTargetSince < OffTargetTicks)
            return;
        data.Locked = false;
        data.LockedTarget = -1;
        data.LockedBodyPoint = -1;
        data.OffTargetSince = -1;
    }

    if (data.Current.TargetSlot < 0)
    {
        StartTrack(slot, data, sample, lagFrames);
        return;
    }
    if (static_cast<int64_t>(sample.ServerTick) - data.Current.LastServerTick != 1)
    {
        data.Current = {};
        StartTrack(slot, data, sample, lagFrames);
        return;
    }

    ++data.Current.Samples;
    int validHypotheses = 0;
    for (int index = 0; index < data.Current.HypothesisCount; ++index)
    {
        Hypothesis& hypothesis = data.Current.Hypotheses[index];
        if (!hypothesis.Valid)
            continue;

        const TargetEvaluation evaluation =
            EvaluateTarget(_shots, sample.Angles, sample.EyePos, *frame,
                           FrameForLag(_shots, sample.ServerTick, lagFrames, hypothesis.LagTicks), slot,
                           data.Current.TargetSlot, data.Current.BodyPoint);
        const float displacement =
            evaluation.Valid ? Geometry::AngularDistance(hypothesis.StartBearing, evaluation.Bearing) : 0.0f;
        if (!evaluation.Valid || !std::isfinite(displacement))
        {
            hypothesis.Valid = false;
            continue;
        }
        ++validHypotheses;
        hypothesis.OnTargetSamples += evaluation.OnTarget();
        hypothesis.MaxTargetDisplacement = std::max(hypothesis.MaxTargetDisplacement, displacement);
    }
    if (validHypotheses == 0)
    {
        data.Current = {};
        StartTrack(slot, data, sample, lagFrames);
        return;
    }

    data.Current.LastServerTick = sample.ServerTick;
    if (sample.ServerTick - data.Current.StartServerTick < TrackingTicks)
        return;

    const Hypothesis* passing = nullptr;
    for (int index = 0; index < data.Current.HypothesisCount; ++index)
    {
        const Hypothesis& hypothesis = data.Current.Hypotheses[index];
        if (!hypothesis.Valid || !MeetsCoverage(hypothesis.OnTargetSamples, data.Current.Samples) ||
            hypothesis.MaxTargetDisplacement < hypothesis.RequiredTargetDisplacement)
            continue;
        if (!passing || hypothesis.OnTargetSamples > passing->OnTargetSamples)
            passing = &hypothesis;
    }
    if (!passing)
    {
        data.Current = {};
        return;
    }
    Count(slot, data, *passing, nowSec);
}

void Aimlock::StartTrack(int slot, SlotData& data, const Sample& sample, const LagFrames& lagFrames)
{
    const PositionFrame* frame = _shots.FindFrame(sample.ServerTick);
    if (!frame)
        return;

    const Candidate candidate = FindCandidate(_shots, sample.Angles, sample.EyePos, *frame, lagFrames, slot);
    if (!candidate.Valid)
        return;

    data.Current = {};
    data.Current.TargetSlot = candidate.TargetSlot;
    data.Current.BodyPoint = candidate.BodyPoint;
    data.Current.StartServerTick = sample.ServerTick;
    data.Current.LastServerTick = sample.ServerTick;
    data.Current.Samples = 1;
    for (int lagTicks = lagFrames.First; lagTicks <= lagFrames.Last; ++lagTicks)
    {
        const TargetEvaluation evaluation =
            EvaluateTarget(_shots, sample.Angles, sample.EyePos, *frame, lagFrames.Frames[lagTicks - lagFrames.First],
                           slot, candidate.TargetSlot, candidate.BodyPoint);
        if (!evaluation.Valid)
            continue;
        data.Current.Hypotheses[data.Current.HypothesisCount++] = {
            .StartBearing = evaluation.Bearing,
            .RequiredTargetDisplacement = evaluation.RequiredDisplacement,
            .LagTicks = lagTicks,
            .OnTargetSamples = evaluation.OnTarget() ? 1 : 0,
            .Valid = true,
        };
    }
    if (data.Current.HypothesisCount == 0)
        data.Current = {};
}

void Aimlock::Count(int slot, SlotData& data, const Hypothesis& hypothesis, double nowSec)
{
    const bool reported = _suspicion.Add(
        slot,
        {.Kind = Kind,
         .Points = PerEpisode,
         .Reason = std::format("A tracking episode stayed on target for {}/{} samples while the target moved {:.1f} "
                               "of {:.1f} required degrees.",
                               hypothesis.OnTargetSamples, data.Current.Samples, hypothesis.MaxTargetDisplacement,
                               hypothesis.RequiredTargetDisplacement)},
        nowSec);

    if (reported)
    {
        // Stay on this target so one continuous lock is one episode, not one per re-evaluation.
        data.Locked = true;
        data.LockedTarget = data.Current.TargetSlot;
        data.LockedBodyPoint = data.Current.BodyPoint;
        data.OffTargetSince = -1;
    }
    data.Current = {};
}

bool Aimlock::IsTracking(int slot) const
{
    return InSlotRange(slot) && _slots[slot].Current.TargetSlot >= 0;
}

}  // namespace Anticheat::Rules
