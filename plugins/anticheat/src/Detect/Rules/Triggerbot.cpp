#include "Detect/Rules/Triggerbot.hpp"

#include "Detect/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace Anticheat::Rules
{

/** Eight weighted points are what this rule reports on alone. */
static constexpr float PerPoint = 1.0f / 8.0f;

static constexpr size_t AimHistorySize = 48;
/** The hull is a box, so a crosshair a little outside its inscribed angle still rests on it. */
static constexpr float HullTolerance = 1.25f;
static constexpr float MinimumDistance = 150.0f;
/** A shot this soon after the previous one is part of a burst, not a reaction. */
static constexpr int BurstGapTicks = 10;
static constexpr int FastReactionTicks = 3;  // 47 ms
static constexpr int SlowReactionTicks = 6;  // 94 ms
static constexpr int FastPoints = 2;
static constexpr int SlowPoints = 1;
/** The crosshair must have been resting: the target walked in, the shooter did not flick. */
static constexpr float StillAimDeg = 2.0f;
static constexpr int StillLeadTicks = 4;

void Triggerbot::SlotData::ClearRuns()
{
    for (auto& row : OnSince)
        row.fill(-1);
}

void Triggerbot::Reset()
{
    _slots = {};
}

void Triggerbot::ClearSlot(int slot)
{
    if (!InSlotRange(slot))
        return;
    _slots[slot] = {};
    // Nobody can rest a crosshair on a seat that just changed hands.
    for (auto& data : _slots)
        data.OnSince[slot].fill(-1);
}

void Triggerbot::OnSimulated(int slot, int32_t serverTick, const AimAngles& angles, const Vec3& eyePos)
{
    if (!InSlotRange(slot))
        return;
    auto& data = _slots[slot];
    data.Pending = {.ServerTick = serverTick, .Angles = angles};
    data.PendingEye = eyePos;
    data.PendingValid = Geometry::IsFinite(angles) && Geometry::IsFinite(eyePos);
}

bool Triggerbot::OnTarget(const Vec3& eye, const AimAngles& angles, const PositionSample& target)
{
    if ((target.Origin - eye).Length() < MinimumDistance)
        return false;
    const Vec3 forward = Geometry::AimForward(angles);
    for (float height : Geometry::BodyHeights)
    {
        const Vec3 point{target.Origin.X, target.Origin.Y, target.Origin.Z + height};
        const float distance = (point - eye).Length();
        if (!std::isfinite(distance) || distance < 1e-3f)
            continue;
        const float error = Geometry::AimErrorDeg(eye, forward, point);
        if (std::isfinite(error) &&
            error <= Geometry::AngularSizeDeg(Geometry::PlayerHalfWidth, distance) * HullTolerance)
            return true;
    }
    return false;
}

void Triggerbot::OnFrame(int slot, int32_t serverTick, bool aliveHuman, const ViewLag& lag)
{
    if (!InSlotRange(slot))
        return;
    auto& data = _slots[slot];
    const bool usable = aliveHuman && lag.Valid && data.PendingValid && data.Pending.ServerTick == serverTick;
    data.PendingValid = false;
    if (!usable)
    {
        data.ClearRuns();
        if (!aliveHuman)
            data.Aim.clear();
        return;
    }

    data.Aim.push_back(data.Pending);
    while (data.Aim.size() > AimHistorySize)
        data.Aim.pop_front();

    const PositionFrame* frame = _shots.FindFrame(serverTick);
    const PositionSample* observer = frame ? &frame->Players[slot] : nullptr;
    if (!observer || !observer->Trackable())
    {
        data.ClearRuns();
        return;
    }

    // One lookup per hypothesis, not per hypothesis per target.
    std::array<const PositionFrame*, LagHypothesisCount> past{};
    for (int index = 0; index < LagHypothesisCount; ++index)
        past[index] = _shots.FindFrame(serverTick - LagHypothesis(lag, index));

    for (int target = 0; target < MaxSlots; ++target)
    {
        const PositionSample& current = frame->Players[target];
        const bool eligible = target != slot && _shots.IsOpponent(observer->Team, current);
        for (int index = 0; index < LagHypothesisCount; ++index)
        {
            int32_t& since = data.OnSince[target][index];
            bool on = false;
            if (eligible)
            {
                // What the client saw: the target where it stood that many ticks ago.
                const PositionSample* seen = past[index] ? &past[index]->Players[target] : nullptr;
                on = seen && seen->Trackable() && OnTarget(data.PendingEye, data.Pending.Angles, *seen);
            }
            since = on ? (since < 0 ? serverTick : since) : -1;
        }
    }
}

void Triggerbot::OnWeaponFire(int slot, int32_t fireTick)
{
    if (!InSlotRange(slot))
        return;
    auto& data = _slots[slot];
    if (fireTick == data.LastFireTick)
        return;
    data.PreviousFireTick = data.LastFireTick;
    data.LastFireTick = fireTick;
}

float Triggerbot::AimTravel(const SlotData& data, int32_t sinceTick, int32_t untilTick)
{
    const AimSample* rest = nullptr;
    float travel = 0.0f;
    for (const AimSample& sample : data.Aim)
    {
        if (sample.ServerTick < sinceTick || sample.ServerTick > untilTick)
            continue;
        if (!rest)
        {
            rest = &sample;
            continue;
        }
        const float moved = Geometry::AngularDistance(rest->Angles, sample.Angles);
        if (!std::isfinite(moved))
            return 180.0f;
        travel = std::max(travel, moved);
    }
    // No history means the rest cannot be confirmed, which reads as motion.
    return rest ? travel : 180.0f;
}

void Triggerbot::OnPlayerHurt(int slot, const ShotView& shot, double nowSec)
{
    const int victim = shot.VictimSlot;
    if (!InSlotRange(slot) || shot.Slot != slot || !shot.HurtSeen || !InSlotRange(victim) || victim == slot)
        return;

    auto& data = _slots[slot];
    const int32_t previousFire = data.LastFireTick == shot.FireTick ? data.PreviousFireTick : data.LastFireTick;
    if (previousFire >= 0 && shot.FireTick - previousFire <= BurstGapTicks)
        return;

    // The slowest hypothesis, so a wrong guess about the client's view can only understate the reaction.
    int reaction = -1;
    for (int index = 0; index < LagHypothesisCount; ++index)
    {
        const int32_t since = data.OnSince[victim][index];
        if (since >= 0)
            reaction = std::max(reaction, shot.FireTick - since);
    }
    if (reaction < 0)
        return;

    const int points = reaction <= FastReactionTicks ? FastPoints : reaction <= SlowReactionTicks ? SlowPoints : 0;
    if (points == 0)
        return;
    if (AimTravel(data, shot.FireTick - reaction - StillLeadTicks, shot.FireTick) > StillAimDeg)
        return;

    _suspicion.Add(
        slot,
        {.Kind = Kind,
         .Points = static_cast<float>(points) * PerPoint,
         .Reason = std::format("A hit landed {} ticks (~{} ms) after the target walked into a resting crosshair.",
                               reaction, static_cast<int>(reaction * 1000.0f / TickRate))},
        nowSec);
}

}  // namespace Anticheat::Rules
