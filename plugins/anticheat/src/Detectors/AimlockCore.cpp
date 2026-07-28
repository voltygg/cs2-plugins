#include "AimlockCore.hpp"

#include "Core/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace Anticheat
{

namespace
{
constexpr int TrackingTicks = static_cast<int>(TickRate * 1.5f);  // 96
constexpr int RearmTicks = static_cast<int>(TickRate * 0.5f);     // 32
constexpr int LagSearchRadius = 2;
constexpr float PlayerHalfWidth = 16.0f;  // the CS2 hull is 32 units wide, measured from its center
constexpr float MinimumDistance = 200.0f;
constexpr float MinimumTargetTravel = 48.0f;  // one and a half player widths, as degrees at that range
constexpr float MaximumInterpolationTicks = 19.0f;
constexpr int DetectionThreshold = 3;
constexpr double EvidenceWindowSec = 600.0;  // 10 minutes

/** 95% of the episode's samples must have been inside the target's angular width. */
constexpr bool MeetsCoverage(int onTarget, int samples)
{
    return samples > 0 && onTarget * 20 >= samples * 19;
}
static_assert(MeetsCoverage(123, 129));
static_assert(!MeetsCoverage(122, 129));

struct TargetEvaluation
{
    AimAngles Bearing;
    float Error = 180.0f;
    float MaximumError = 0.0f;
    float RequiredDisplacement = 0.0f;
    bool Valid = false;

    bool OnTarget() const { return Valid && Error <= MaximumError; }
};

TargetEvaluation EvaluateTarget(const ShotCorrelatorCore& shots, const AimAngles& angles, const Vec3& eyePos,
                                int32_t serverTick, const PositionFrame& currentFrame, int observerSlot, int targetSlot,
                                int bodyPoint, int lagTicks)
{
    TargetEvaluation result;
    if (!InSlotRange(observerSlot) || !InSlotRange(targetSlot) || bodyPoint < 0 ||
        bodyPoint >= Geometry::BodyPointCount || lagTicks < 0 || !Geometry::IsFinite(eyePos) ||
        !Geometry::IsFinite(angles))
        return result;

    const PositionSample& observer = currentFrame.Players[observerSlot];
    const PositionSample& currentTarget = currentFrame.Players[targetSlot];
    const PositionFrame* historical = shots.FindFrame(serverTick - lagTicks);
    if (!observer.Valid || !observer.Alive || observer.Teleported || !currentTarget.Valid || !currentTarget.Alive ||
        currentTarget.Teleported || !shots.AreOpponents(observer.Team, currentTarget.Team) || !historical)
        return result;

    const PositionSample& target = historical->Players[targetSlot];
    if (!target.Valid || !target.Alive || target.Teleported || !shots.AreOpponents(observer.Team, target.Team) ||
        (target.Origin - eyePos).Length() < MinimumDistance)
        return result;

    const Vec3 point = {target.Origin.X, target.Origin.Y, target.Origin.Z + Geometry::BodyHeights[bodyPoint]};
    const float distance = (point - eyePos).Length();
    if (!Geometry::IsFinite(point) || !std::isfinite(distance) || distance < 1e-3f)
        return result;

    result.Error = Geometry::AimErrorDeg(eyePos, angles, point);
    result.MaximumError = Geometry::AngularSizeDeg(PlayerHalfWidth, distance);
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

/** The single target the aim is already inside; two candidates means no episode. */
Candidate FindCandidate(const ShotCorrelatorCore& shots, const AimAngles& angles, const Vec3& eyePos,
                        int32_t serverTick, const PositionFrame& frame, int observerSlot, const LagEstimate& lag)
{
    Candidate best;
    if (!lag.Valid)
        return best;

    int matchedTarget = -1;
    bool ambiguous = false;
    for (int lagTicks = std::max(0, lag.Ticks - LagSearchRadius); lagTicks <= lag.Ticks + LagSearchRadius; ++lagTicks)
    {
        for (int targetSlot = 0; targetSlot < MaxSlots; ++targetSlot)
        {
            if (targetSlot == observerSlot)
                continue;
            for (int bodyPoint = 0; bodyPoint < Geometry::BodyPointCount; ++bodyPoint)
            {
                const TargetEvaluation evaluation = EvaluateTarget(shots, angles, eyePos, serverTick, frame,
                                                                   observerSlot, targetSlot, bodyPoint, lagTicks);
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
}  // namespace

LagEstimate EstimateVisualLag(float rttSeconds, float interpRatio)
{
    LagEstimate estimate;
    if (!std::isfinite(rttSeconds) || rttSeconds < 0.0f || rttSeconds > 2.0f || !std::isfinite(interpRatio) ||
        interpRatio < 0.0f || interpRatio > MaximumInterpolationTicks)
        return estimate;

    const float interpTicks = interpRatio == 0.0f ? 1.0f : interpRatio;
    estimate.Ticks = std::max(0, static_cast<int>(std::lround(rttSeconds * TickRate + interpTicks)));
    estimate.RoundTripMs = rttSeconds * 1000.0f;
    estimate.InterpTicks = interpTicks;
    estimate.Valid = true;
    return estimate;
}

void AimlockCore::Reset()
{
    _slots = {};
    _incidents = {};
}

void AimlockCore::OnSlotChanged(int slot)
{
    if (!InSlotRange(slot))
        return;
    _slots[slot] = {};
    _incidents[slot].clear();
}

void AimlockCore::OnSimulated(int slot, int32_t serverTick, const AimAngles& angles, const Vec3& eyePos)
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

std::optional<Finding> AimlockCore::OnFrame(int slot, int32_t serverTick, bool aliveHuman, const LagEstimate& lag,
                                            double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot))
        return out;

    auto& data = _slots[slot];
    if (!aliveHuman)
    {
        // Only the tracking state is unusable across a death - counted episodes stay in _incidents,
        // so dying between them cannot wipe the evidence.
        data = {};
        return out;
    }
    if (!data.Pending.Valid)
        return out;

    const Sample sample = data.Pending;
    data.Pending = {};
    // The simulated command and the world snapshot must describe one and the same new tick.
    if (sample.ServerTick != serverTick || sample.ServerTick == data.LastProcessedTick || !_shots.FindFrame(serverTick))
    {
        data.Current = {};
        return out;
    }
    data.LastProcessedTick = sample.ServerTick;
    Evaluate(slot, data, sample, lag, nowSec, out);
    return out;
}

void AimlockCore::Evaluate(int slot, SlotData& data, const Sample& sample, const LagEstimate& lag, double nowSec,
                           std::optional<Finding>& out)
{
    const PositionFrame* frame = _shots.FindFrame(sample.ServerTick);
    if (!frame)
    {
        data.Current = {};
        return;
    }

    if (data.Latched)
    {
        // After a detection, stay quiet until the player leaves the target for half a second.
        bool stillLocked = false;
        if (lag.Valid)
            for (int lagTicks = std::max(0, lag.Ticks - LagSearchRadius);
                 !stillLocked && lagTicks <= lag.Ticks + LagSearchRadius; ++lagTicks)
                stillLocked = EvaluateTarget(_shots, sample.Angles, sample.EyePos, sample.ServerTick, *frame, slot,
                                             data.LatchedTarget, data.LatchedBodyPoint, lagTicks)
                                  .OnTarget();
        if (stillLocked)
        {
            data.BreakStartTick = -1;
            return;
        }
        if (data.BreakStartTick < 0)
            data.BreakStartTick = sample.ServerTick;
        if (static_cast<int64_t>(sample.ServerTick) - data.BreakStartTick < RearmTicks)
            return;
        data.Latched = false;
        data.LatchedTarget = -1;
        data.LatchedBodyPoint = -1;
        data.BreakStartTick = -1;
    }

    if (data.Current.TargetSlot < 0)
    {
        StartTrack(slot, data, sample, lag);
        return;
    }
    if (static_cast<int64_t>(sample.ServerTick) - data.Current.LastServerTick != 1)
    {
        data.Current = {};
        StartTrack(slot, data, sample, lag);
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
            EvaluateTarget(_shots, sample.Angles, sample.EyePos, sample.ServerTick, *frame, slot,
                           data.Current.TargetSlot, data.Current.BodyPoint, hypothesis.LagTicks);
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
        StartTrack(slot, data, sample, lag);
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
    Count(slot, data, *passing, nowSec, out);
}

void AimlockCore::StartTrack(int slot, SlotData& data, const Sample& sample, const LagEstimate& lag)
{
    const PositionFrame* frame = _shots.FindFrame(sample.ServerTick);
    if (!frame)
        return;

    const Candidate candidate =
        FindCandidate(_shots, sample.Angles, sample.EyePos, sample.ServerTick, *frame, slot, lag);
    if (!candidate.Valid)
        return;

    data.Current = {};
    data.Current.TargetSlot = candidate.TargetSlot;
    data.Current.BodyPoint = candidate.BodyPoint;
    data.Current.StartServerTick = sample.ServerTick;
    data.Current.LastServerTick = sample.ServerTick;
    data.Current.Samples = 1;
    for (int lagTicks = std::max(0, lag.Ticks - LagSearchRadius);
         lagTicks <= lag.Ticks + LagSearchRadius &&
         data.Current.HypothesisCount < static_cast<int>(data.Current.Hypotheses.size());
         ++lagTicks)
    {
        const TargetEvaluation evaluation =
            EvaluateTarget(_shots, sample.Angles, sample.EyePos, sample.ServerTick, *frame, slot, candidate.TargetSlot,
                           candidate.BodyPoint, lagTicks);
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

void AimlockCore::Count(int slot, SlotData& data, const Hypothesis& hypothesis, double nowSec,
                        std::optional<Finding>& out)
{
    std::deque<double>& incidents = _incidents[slot];
    while (!incidents.empty() && nowSec - incidents.front() >= EvidenceWindowSec)
        incidents.pop_front();
    incidents.push_back(nowSec);

    if (static_cast<int>(incidents.size()) >= DetectionThreshold)
    {
        out = Finding{.Kind = DetectionKind::Aimlock,
                      .Evidence = std::format("{} precise tracking episodes; the latest stayed on target for {}/{} "
                                              "samples while the target moved {:.1f} of {:.1f} required degrees.",
                                              incidents.size(), hypothesis.OnTargetSamples, data.Current.Samples,
                                              hypothesis.MaxTargetDisplacement, hypothesis.RequiredTargetDisplacement)};
        incidents.clear();
        data.Latched = true;
        data.LatchedTarget = data.Current.TargetSlot;
        data.LatchedBodyPoint = data.Current.BodyPoint;
        data.BreakStartTick = -1;
    }
    data.Current = {};
}

int AimlockCore::IncidentCount(int slot) const
{
    return InSlotRange(slot) ? static_cast<int>(_incidents[slot].size()) : 0;
}

bool AimlockCore::IsTracking(int slot) const
{
    return InSlotRange(slot) && _slots[slot].Current.TargetSlot >= 0;
}

}  // namespace Anticheat
