#include "SilentAimCore.hpp"

#include "Core/Geometry.hpp"
#include "Core/WeaponClass.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace Anticheat
{

namespace
{
constexpr int DetectionScore = 12;
constexpr double EvidenceWindowSec = 600.0;  // 10 minutes
constexpr float MinimumImpactDistance = 100.0f;
constexpr float MaximumImpactDistance = 10000.0f;

// A deviation this large is beyond every weapon's ceiling, so it is worth more than a marginal one.
constexpr float BlatantDeviation = 22.5f;
constexpr int BlatantPoints = 3;
constexpr int AirbornePoints = 1;  // inaccuracy while jumping makes a wide shot cheap evidence
constexpr int GroundedPoints = 2;
}  // namespace

void SilentAimCore::Reset()
{
    _incidents = {};
}

void SilentAimCore::OnSlotChanged(int slot)
{
    if (InSlotRange(slot))
        _incidents[slot].clear();
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
    while (!incidents.empty() && nowSec - incidents.front().Time >= EvidenceWindowSec)
        incidents.pop_front();
    incidents.push_back({nowSec, points});

    int total = 0;
    for (const auto& incident : incidents)
        total += incident.Points;
    if (total < DetectionScore)
        return out;

    out = Finding{.Kind = DetectionKind::SilentAim,
                  .Evidence = std::format("{:.2f} degrees from visible aim with {} added {} points; the rolling score "
                                          "reached {}/{}.",
                                          shot.SilentMaxDeviation, shot.Weapon, points, total, DetectionScore)};
    incidents.clear();
    return out;
}

int SilentAimCore::Score(int slot, double nowSec) const
{
    if (!InSlotRange(slot))
        return 0;
    int total = 0;
    for (const auto& incident : _incidents[slot])
        if (nowSec - incident.Time < EvidenceWindowSec)
            total += incident.Points;
    return total;
}

}  // namespace Anticheat
