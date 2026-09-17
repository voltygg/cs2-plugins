#pragma once

#include "Detect/Samples.hpp"
#include "Detect/ShotHistory.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Hooks/Api.hpp>
#include <array>
#include <optional>

namespace Anticheat
{

class SightLines
{
public:
    explicit SightLines(VoltMod::Runtime& runtime) : _rt(runtime) {}

    /** Why sight lines cannot be traced this map, when they cannot. */
    VoltMod::Status Available() const;

    /**
     * For every slot in @p viewers, trace the sight line to the opponent nearest its aim and record
     * the answer in that opponent's sample. @p aims are the viewers' current eye angles.
     */
    void Trace(std::array<PositionSample, MaxSlots>& players, const std::array<AimAngles, MaxSlots>& aims,
               const std::array<bool, MaxSlots>& viewers, const ShotHistory& teams) const;

    /** Whether @p viewer has line of sight to any body point of @p target right now. */
    std::optional<bool> CanSee(int viewer, int target) const;

private:
    std::optional<bool> Trace(const VoltMod::Pawn& viewer, const Vec3& eye, const VoltMod::Pawn& target,
                              const Vec3& feet) const;

    VoltMod::Runtime& _rt;
};

}  // namespace Anticheat
