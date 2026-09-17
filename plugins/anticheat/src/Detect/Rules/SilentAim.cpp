#include "Detect/Rules/SilentAim.hpp"

#include "Detect/Geometry.hpp"
#include "Detect/WeaponClass.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace Anticheat::Rules
{

static constexpr float MinimumImpactDistance = 100.0f;
static constexpr float MaximumImpactDistance = 10000.0f;

// Beyond every weapon's ceiling, so worth more than a marginal deviation.
static constexpr float BlatantDeviation = 22.5f;
static constexpr int BlatantPoints = 3;
static constexpr int AirbornePoints = 1;  // inaccuracy while jumping makes a wide shot cheap evidence
static constexpr int GroundedPoints = 2;

void SilentAim::OnShotUpdated(int slot, ShotView& shot)
{
    if (!InSlotRange(slot) || shot.Slot != slot || shot.SilentMeasured || shot.Finalized ||
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

std::optional<Finding> SilentAim::Finalize(int slot, ShotView& shot, double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot) || !shot.HurtSeen || !shot.ImpactSeen)
        return out;

    const float threshold = SilentAimDeviationThreshold(shot.Weapon);
    if (!std::isfinite(shot.SilentMaxDeviation) || shot.SilentMaxDeviation <= threshold)
        return out;

    const int points = (shot.SilentMaxDeviation > BlatantDeviation ? BlatantPoints
                        : shot.Airborne                            ? AirbornePoints
                                                                   : GroundedPoints) +
                       static_cast<int>(shot.Headshot) + static_cast<int>(shot.Wallbang);

    return _suspicion.Add(slot,
                          {.Kind = Kind,
                           .Points = static_cast<float>(points),
                           .Reason = std::format("{:.2f} degrees from visible aim with {} added {} points.",
                                                 shot.SilentMaxDeviation, shot.Weapon, points)},
                          nowSec);
}

}  // namespace Anticheat::Rules
