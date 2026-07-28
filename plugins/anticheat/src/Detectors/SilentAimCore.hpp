#pragma once

// Compares where a shot visibly pointed against where its bullet actually landed. Legitimately the
// two differ by spread and recoil only; writing the fire angle separately from the view opens a gap
// no weapon can explain. SDK-free.

#include "Core/Finding.hpp"
#include "Core/Samples.hpp"

#include <array>
#include <deque>
#include <optional>

namespace Anticheat
{

class SilentAimCore
{
public:
    void Reset();
    void OnSlotChanged(int slot);

    /** Measure the shot once its impact point is known. Safe to call repeatedly. */
    void OnShotUpdated(int slot, ShotView& shot);

    /**
     * Score a shot that is now two ticks old, so every event it could produce has arrived. Only
     * shots that both hurt someone and reported an impact are judged; the rest are dropped.
     */
    std::optional<Finding> Finalize(int slot, ShotView& shot, double nowSec);

    int Score(int slot, double nowSec) const;

private:
    struct Incident
    {
        double Time = 0.0;
        int Points = 0;
    };

    std::array<std::deque<Incident>, MaxSlots> _incidents{};
};

}  // namespace Anticheat
