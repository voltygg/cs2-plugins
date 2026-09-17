#include "Detect/Rules/Recoil.hpp"

#include "Detect/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <utility>

namespace Anticheat::Rules
{

/** A 30-shot rifle spray spans about 190 ticks, and every shot needs the command after it. */
static constexpr int MaxShotGapTicks = 16;
static constexpr size_t MinSprayShots = 8;
static constexpr size_t MaxSprayShots = 30;
/** Below this much total punch there is nothing worth cancelling. */
static constexpr float MinPunchTravelDeg = 3.0f;
static constexpr float MaxResidualDeg = 0.35f;
static constexpr float MinSlope = 0.5f;
static constexpr float MaxSlope = 2.5f;

void Recoil::Reset()
{
    _slots = {};
}

void Recoil::OnSlotChanged(int slot)
{
    if (!InSlotRange(slot))
        return;
    _slots[slot] = {};
}

void Recoil::OnCommand(int slot, const CmdSample& cmd)
{
    if (!InSlotRange(slot) || !cmd.BaseAnglesFinite)
        return;

    _slots[slot].Commands.Push({.CmdNum = cmd.CmdNum,
                                .View = cmd.BaseAngles(),
                                .Punch = cmd.Punch,
                                .HasPunch = cmd.HasPunch && Geometry::IsFinite(cmd.Punch)});
}

const Recoil::Command* Recoil::Find(const SlotData& data, int32_t cmdNum) const
{
    return data.Commands.Find(cmdNum);
}

std::optional<Finding> Recoil::OnShot(int slot, const ShotView& shot, double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot) || shot.Slot != slot)
        return out;

    auto& data = _slots[slot];
    if (!data.Spray.empty())
    {
        const Shot& last = data.Spray.back();
        // A burst counter that stopped climbing means the engine saw a pause, whatever the ticks say.
        const bool restarted = shot.ShotsFired > 0 && last.ShotsFired > 0 && shot.ShotsFired <= last.ShotsFired;
        if (data.Weapon != shot.Weapon || shot.FireTick - last.FireTick > MaxShotGapTicks || restarted)
            out = Finalize(slot, data, nowSec);
    }

    data.Weapon = shot.Weapon;
    data.Spray.push_back({.CmdNum = shot.CmdNum, .FireTick = shot.FireTick, .ShotsFired = shot.ShotsFired});
    if (data.Spray.size() >= MaxSprayShots)
    {
        std::optional<Finding> closed = Finalize(slot, data, nowSec);
        if (!out)
            out = std::move(closed);
    }
    return out;
}

std::optional<Finding> Recoil::OnFrame(int slot, int32_t serverTick, double nowSec)
{
    if (!InSlotRange(slot))
        return std::nullopt;
    auto& data = _slots[slot];
    if (data.Spray.empty() || serverTick - data.Spray.back().FireTick <= MaxShotGapTicks)
        return std::nullopt;
    return Finalize(slot, data, nowSec);
}

SprayFit Recoil::Fit(const SlotData& data, int viewLag) const
{
    struct Pair
    {
        float PunchPitch, PunchYaw, ViewPitch, ViewYaw;
    };
    std::vector<Pair> pairs;
    pairs.reserve(data.Spray.size());

    SprayFit fit;
    float sxx = 0.0f;
    float sxy = 0.0f;
    for (size_t k = 0; k + 1 < data.Spray.size(); ++k)
    {
        // The punch a shot leaves behind is what the client predicted for the command after it.
        const Command* punchA = Find(data, data.Spray[k].CmdNum + 1);
        const Command* punchB = Find(data, data.Spray[k + 1].CmdNum + 1);
        const Command* viewA = Find(data, data.Spray[k].CmdNum + viewLag);
        const Command* viewB = Find(data, data.Spray[k + 1].CmdNum + viewLag);
        if (!punchA || !punchB || !viewA || !viewB || !punchA->HasPunch || !punchB->HasPunch)
            continue;

        const Pair pair{punchB->Punch.Pitch - punchA->Punch.Pitch, Geometry::YawDelta(punchA->Punch.Yaw, punchB->Punch.Yaw),
                        viewB->View.Pitch - viewA->View.Pitch, Geometry::YawDelta(viewA->View.Yaw, viewB->View.Yaw)};
        if (!std::isfinite(pair.PunchPitch) || !std::isfinite(pair.PunchYaw) || !std::isfinite(pair.ViewPitch) ||
            !std::isfinite(pair.ViewYaw))
            continue;
        pairs.push_back(pair);
        sxx += pair.PunchPitch * pair.PunchPitch + pair.PunchYaw * pair.PunchYaw;
        sxy += pair.PunchPitch * pair.ViewPitch + pair.PunchYaw * pair.ViewYaw;
        fit.PunchTravelDeg += std::hypot(pair.PunchPitch, pair.PunchYaw);
    }

    if (pairs.size() + 1 < MinSprayShots || fit.PunchTravelDeg < MinPunchTravelDeg || sxx <= 0.0f)
        return fit;

    // Least squares for view = -slope * punch, both axes pooled.
    fit.Slope = -sxy / sxx;
    float residual = 0.0f;
    for (const Pair& pair : pairs)
    {
        const float pitch = pair.ViewPitch + fit.Slope * pair.PunchPitch;
        const float yaw = pair.ViewYaw + fit.Slope * pair.PunchYaw;
        residual += pitch * pitch + yaw * yaw;
    }
    fit.ResidualDeg = std::sqrt(residual / static_cast<float>(pairs.size()));
    fit.Valid = std::isfinite(fit.Slope) && std::isfinite(fit.ResidualDeg);
    return fit;
}

std::optional<Finding> Recoil::Finalize(int slot, SlotData& data, double nowSec)
{
    const size_t shots = data.Spray.size();
    const std::string weapon = data.Weapon;
    if (shots < MinSprayShots)
    {
        data.Spray.clear();
        return std::nullopt;
    }

    // The view may cancel the punch in the same command or the one after; take the better fit.
    SprayFit best;
    for (int viewLag = 0; viewLag <= 1; ++viewLag)
    {
        const SprayFit fit = Fit(data, viewLag);
        if (fit.Valid && (!best.Valid || fit.ResidualDeg < best.ResidualDeg))
            best = fit;
    }
    data.Spray.clear();

    if (!best.Valid || best.Slope < MinSlope || best.Slope > MaxSlope || best.ResidualDeg > MaxResidualDeg)
        return std::nullopt;

    return _suspicion.Add(
        slot,
        {.Kind = Kind,
         .Points = 1.0f,
         .Reason = std::format("A spray of {} shots of {} followed {:.1f} degrees of punch with factor {:.2f} and "
                               "{:.2f} degrees of residual.",
                               shots, weapon, best.PunchTravelDeg, best.Slope, best.ResidualDeg)},
        nowSec);
}

bool Recoil::InSpray(int slot) const
{
    return InSlotRange(slot) && !_slots[slot].Spray.empty();
}

}  // namespace Anticheat::Rules
