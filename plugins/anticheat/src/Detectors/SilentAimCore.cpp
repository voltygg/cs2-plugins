#include "SilentAimCore.hpp"

#include "Core/Geometry.hpp"
#include "Core/WeaponClass.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace Anticheat
{

static constexpr int DetectionScore = 12;
static constexpr float MinimumImpactDistance = 100.0f;
static constexpr float MaximumImpactDistance = 10000.0f;

// Beyond every weapon's ceiling, so worth more than a marginal deviation.
static constexpr float BlatantDeviation = 22.5f;
static constexpr int BlatantPoints = 3;
static constexpr int AirbornePoints = 1;  // inaccuracy while jumping makes a wide shot cheap evidence
constexpr int GroundedPoints = 2;

void SilentAimCore::Reset()
{
    _incidents = {};
}

void SilentAimCore::OnSlotChanged(int slot)
{
    if (InSlotRange(slot))
        _incidents[slot].Clear();
}

void SilentAimCore::OnShotUpdated(int slot, ShotView& shot)
{
    if (!InSlotRange(slot) || shot.Slot != slot || shot.SilentMeasured || shot.SilentConsumed ||
        !shot.HasVisibleAngles || !shot.ImpactSeen || !Geometry::IsFinite(shot.EyePos) ||
        !Geometry::IsFinite(shot.ImpactPos) || !Geometry::IsFinite(shot.VisibleAngles))
        return;

    const float distance = (shot.ImpactPos - shot.EyePos).Length();
    if (!std::isfinite(distance) || distance < MinimumImpactDistance || distance > MaximumImpactDistance)
        return;

    const float deviation = Geometry::AimErrorDeg(shot.EyePos, shot.VisibleAngles, shot.ImpactPos);
    if (!std::isfinite(deviation))
        return;

    shot.SilentMeasured = true;
    shot.SilentMaxDeviation = std::max(shot.SilentMaxDeviation, deviation);
}

std::optional<Finding> SilentAimCore::Finalize(int slot, ShotView& shot, double nowSec)
{
    std::optional<Finding> out;
    shot.SilentConsumed = true;
    if (!InSlotRange(slot) || !shot.HurtSeen || !shot.ImpactSeen)
        return out;

    const float threshold = SilentAimDeviationThreshold(shot.Weapon);
    if (!std::isfinite(shot.SilentMaxDeviation) || shot.SilentMaxDeviation <= threshold)
        return out;

    const int points = (shot.SilentMaxDeviation > BlatantDeviation ? BlatantPoints
                        : shot.Airborne                            ? AirbornePoints
                                                                   : GroundedPoints) +
                       static_cast<int>(shot.Headshot) + static_cast<int>(shot.Wallbang);

    auto& incidents = _incidents[slot];
    const int total = incidents.Add(nowSec, points);
    if (total < DetectionScore)
        return out;

    out = Finding{.Kind = DetectionKind::SilentAim,
                  .Evidence = std::format("{:.2f} degrees from visible aim with {} added {} points; the rolling score "
                                          "reached {}/{}.",
                                          shot.SilentMaxDeviation, shot.Weapon, points, total, DetectionScore)};
    incidents.Clear();
    return out;
}

int SilentAimCore::Score(int slot, double nowSec) const
{
    return InSlotRange(slot) ? _incidents[slot].Value(nowSec) : 0;
}

}  // namespace Anticheat
