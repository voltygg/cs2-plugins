#include "Detect/Rules/MouseMismatch.hpp"

#include "Detect/Geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>

namespace Anticheat::Rules
{

static constexpr size_t RatioHistorySize = 32;
static constexpr size_t RatiosToCalibrate = 16;
/** Counts and degrees below these carry too much rounding to teach a scale from. */
static constexpr int MinLearnCounts = 4;
static constexpr float MinLearnDeg = 0.05f;
static constexpr float ConsistentSpread = 0.25f;
static constexpr float ConsistentShare = 0.75f;
/** A turn is unexplained past this many counts' worth, and never below one degree. */
static constexpr float SlackCounts = 4.0f;
static constexpr float MinUnexplainedDeg = 1.0f;
/** The unexplained turn must have brought the aim onto an enemy to count. */
static constexpr float ConvergedDeg = 3.0f;
static constexpr float ConvergenceShare = 0.5f;
static constexpr float MinimumDistance = 100.0f;

void MouseMismatch::Axis::Learn(float ratio)
{
    Ratios.push_back(ratio);
    while (Ratios.size() > RatioHistorySize)
        Ratios.pop_front();
    Ready = false;
    if (Ratios.size() < RatiosToCalibrate)
        return;

    // Learning runs on every command, so the median stays on the stack and stops at the middle.
    std::array<float, RatioHistorySize> buffer{};
    const size_t count = Ratios.size();
    std::copy(Ratios.begin(), Ratios.end(), buffer.begin());
    const auto middle = buffer.begin() + count / 2;
    std::nth_element(buffer.begin(), middle, buffer.begin() + count);
    const float median = *middle;
    if (median == 0.0f || !std::isfinite(median))
        return;

    // A controller, or a client that does not fill the counts, never agrees with itself.
    size_t consistent = 0;
    for (float value : Ratios)
        consistent += std::abs(value - median) <= std::abs(median) * ConsistentSpread;
    if (static_cast<float>(consistent) >= static_cast<float>(count) * ConsistentShare)
    {
        Scale = median;
        Ready = true;
    }
}

void MouseMismatch::Reset()
{
    _slots = {};
}

void MouseMismatch::OnSlotChanged(int slot)
{
    if (!InSlotRange(slot))
        return;
    _slots[slot] = {};
}

std::optional<float> MouseMismatch::NearestOpponentError(int slot, int32_t serverTick, const Vec3& eye,
                                                     const AimAngles& angles) const
{
    // The frame for this tick is captured after the command, so the previous one is the newest.
    const PositionFrame* frame = _shots.FindFrame(serverTick - 1);
    if (!frame || !frame->Players[slot].Valid)
        return std::nullopt;

    const int team = frame->Players[slot].Team;
    std::optional<float> best;
    for (int target = 0; target < MaxSlots; ++target)
    {
        const PositionSample& player = frame->Players[target];
        if (target == slot || !_shots.IsOpponent(team, player) || (player.Origin - eye).Length() < MinimumDistance)
            continue;
        const float error = Geometry::NearestBodyAimError(eye, angles, player.Origin);
        if (std::isfinite(error) && (!best || error < *best))
            best = error;
    }
    return best;
}

std::optional<Finding> MouseMismatch::OnSimulated(int slot, const CmdSample& cmd, int32_t serverTick, bool recentlyTeleported,
                                              double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot))
        return out;

    auto& data = _slots[slot];
    const CmdSample last = data.Last;
    const bool consecutive = data.HasLast && cmd.CmdNum == last.CmdNum + 1 && serverTick - data.LastTick <= 1;
    data.Last = cmd;
    data.LastTick = serverTick;
    data.HasLast = cmd.BaseAnglesFinite;
    if (!consecutive || !cmd.BaseAnglesFinite || recentlyTeleported)
        return out;

    // Keyboard turning and scope zoom both change the degrees a count is worth.
    const bool keyboardTurn = ((cmd.Buttons | last.Buttons) & (ButtonTurnLeft | ButtonTurnRight)) != 0;
    if (keyboardTurn || cmd.Scoped || last.Scoped)
        return out;

    const float yawTurn = Geometry::YawDelta(last.ViewYaw, cmd.ViewYaw);
    const float pitchTurn = cmd.ViewPitch - last.ViewPitch;
    if (!std::isfinite(yawTurn) || !std::isfinite(pitchTurn))
        return out;

    if (std::abs(cmd.MouseDx) >= MinLearnCounts && std::abs(yawTurn) >= MinLearnDeg)
        data.Yaw.Learn(yawTurn / static_cast<float>(cmd.MouseDx));
    if (std::abs(cmd.MouseDy) >= MinLearnCounts && std::abs(pitchTurn) >= MinLearnDeg)
        data.Pitch.Learn(pitchTurn / static_cast<float>(cmd.MouseDy));
    if (!data.Yaw.Ready)
        return out;

    const auto unexplained = [](float turn, int counts, const Axis& axis) {
        const float residual = std::abs(turn - static_cast<float>(counts) * axis.Scale);
        const float slack = std::max(MinUnexplainedDeg, SlackCounts * std::abs(axis.Scale));
        return residual > slack ? residual : 0.0f;
    };
    const float yawResidual = unexplained(yawTurn, cmd.MouseDx, data.Yaw);
    const float pitchResidual = data.Pitch.Ready ? unexplained(pitchTurn, cmd.MouseDy, data.Pitch) : 0.0f;
    const float residual = std::hypot(yawResidual, pitchResidual);
    if (residual <= 0.0f)
        return out;

    // Only a turn that arrived on an enemy is evidence; the rest is a lost packet or a hiccup.
    const std::optional<float> before = NearestOpponentError(slot, serverTick, last.EyePos, last.BaseAngles());
    const std::optional<float> after = NearestOpponentError(slot, serverTick, cmd.EyePos, cmd.BaseAngles());
    if (!before || !after || *after > ConvergedDeg || *before - *after < residual * ConvergenceShare)
        return out;

    return _suspicion.Add(
        slot,
        {.Kind = Kind,
         .Points = 1.0f,
         .Reason = std::format("A turn the mouse could not explain landed on an enemy: {:.2f} degrees on {}/{} "
                               "counts (scale {:.4f} deg/count), closing the aim from {:.2f} to {:.2f} degrees.",
                               std::hypot(yawTurn, pitchTurn), cmd.MouseDx, cmd.MouseDy, data.Yaw.Scale, *before,
                               *after)},
        nowSec);
}

bool MouseMismatch::Calibrated(int slot) const
{
    return InSlotRange(slot) && _slots[slot].Yaw.Ready;
}

}  // namespace Anticheat::Rules
