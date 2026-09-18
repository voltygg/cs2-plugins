#include "Engine/SightLines.hpp"

#include "Detect/Geometry.hpp"

#include <VoltMod/Core/Slots/Slot.hpp>
#include <cmath>
#include <mathlib/vector.h>

namespace Anticheat
{

/** Enemies further from the crosshair than this are not what the player is looking at. */
static constexpr float CandidateConeDeg = 15.0f;

static Vector ToVector(const Vec3& v)
{
    return {v.X, v.Y, v.Z};
}

VoltMod::Status SightLines::Available() const
{
    return _rt.World.Trace.Available();
}

std::optional<bool> SightLines::Trace(const VoltMod::Pawn& viewer, const Vec3& eye, const VoltMod::Pawn& target,
                                      const Vec3& feet) const
{
    const VoltMod::TraceOptions options{.Ignore1 = viewer.Raw(), .Ignore2 = target.Raw()};
    for (float height : Geometry::BodyHeights)
    {
        const auto clear = _rt.World.Trace.Clear(ToVector(eye), ToVector({feet.X, feet.Y, feet.Z + height}), options);
        if (!clear)
            return std::nullopt;
        if (*clear)
            return true;
    }
    return false;
}

void SightLines::StampVisibility(std::array<PositionSample, MaxSlots>& players,
                                 const std::array<AimAngles, MaxSlots>& aims, const std::array<bool, MaxSlots>& viewers,
                                 const ShotHistory& teams) const
{
    if (!Available())
        return;

    for (int viewer = 0; viewer < MaxSlots; ++viewer)
    {
        const PositionSample& self = players[viewer];
        if (!viewers[viewer] || !self.InPlay() || !Geometry::IsFinite(aims[viewer]))
            continue;

        const Vec3 forward = Geometry::AimForward(aims[viewer]);
        int nearest = -1;
        float nearestError = CandidateConeDeg;
        for (int target = 0; target < MaxSlots; ++target)
        {
            const PositionSample& other = players[target];
            if (target == viewer || !other.InPlay() || !teams.AreOpponents(self.Team, other.Team))
                continue;
            const float error = Geometry::NearestBodyAimErrorAlong(self.EyePos, forward, other.Origin);
            if (std::isfinite(error) && error < nearestError)
            {
                nearest = target;
                nearestError = error;
            }
        }
        if (nearest < 0)
            continue;

        const VoltMod::Pawn viewerPawn = _rt.Entities.PawnOf(viewer);
        const VoltMod::Pawn targetPawn = _rt.Entities.PawnOf(nearest);
        if (!viewerPawn || !targetPawn)
            continue;
        const std::optional<bool> seen = Trace(viewerPawn, self.EyePos, targetPawn, players[nearest].Origin);
        if (!seen)
            continue;
        players[nearest].CheckedBy |= SlotBit(viewer);
        if (*seen)
            players[nearest].SeenBy |= SlotBit(viewer);
    }
}

std::optional<bool> SightLines::CanSee(int viewer, int target) const
{
    if (!VoltMod::IsValidSlot(viewer) || !VoltMod::IsValidSlot(target) || viewer == target)
        return std::nullopt;
    const VoltMod::Pawn viewerPawn = _rt.Entities.PawnOf(viewer);
    const VoltMod::Pawn targetPawn = _rt.Entities.PawnOf(target);
    if (!viewerPawn || !targetPawn || !viewerPawn.IsAlive() || !targetPawn.IsAlive())
        return std::nullopt;

    const Vector eye = viewerPawn.EyePosition();
    const Vector feet = targetPawn.Origin();
    return Trace(viewerPawn, {eye.x, eye.y, eye.z}, targetPawn, {feet.x, feet.y, feet.z});
}

}  // namespace Anticheat
