#pragma once

#include "Detect/Evidence.hpp"
#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"

#include <array>
#include <optional>

namespace Anticheat
{

class SilentAimCore
{
public:
    /** The settings toggle and catalog entry this core reports under. */
    static constexpr DetectionKind Kind = DetectionKind::SilentAim;

    void Reset();
    void OnSlotChanged(int slot);

    /** Measure the shot once its impact point is known. Safe to call repeatedly. */
    void OnShotUpdated(int slot, ShotView& shot);

    /**
     * Score a shot old enough that every event it could produce has arrived. Only shots that both
     * hurt someone and reported an impact are judged; the rest are dropped.
     */
    std::optional<Finding> Finalize(int slot, ShotView& shot, double nowSec);

    int Score(int slot, double nowSec) const;

private:
    /** Weighted: a blatant deviation, a headshot and a wallbang each count for more than one shot. */
    std::array<LongEvidenceWindow, MaxSlots> _incidents{};
};

}  // namespace Anticheat
