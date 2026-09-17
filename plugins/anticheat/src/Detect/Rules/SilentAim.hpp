#pragma once

#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"
#include "Detect/Suspicion.hpp"


namespace Anticheat::Rules
{

/** Holds nothing per player: the measurement rides on the shot and the evidence on the score. */
class SilentAim
{
public:
    /** The settings toggle and catalog entry this rule reports under. */
    static constexpr DetectionKind Kind = DetectionKind::SilentAim;

    explicit SilentAim(Suspicion& suspicion) : _suspicion(suspicion) {}

    /** Measure the shot once its impact point is known. Safe to call repeatedly. */
    void OnShotUpdated(int slot, ShotView& shot);

    /**
     * Score a shot old enough that every event it could produce has arrived. Only shots that both
     * hurt someone and reported an impact are judged; the rest are dropped.
     */
    void Finalize(int slot, ShotView& shot, double nowSec);

private:
    Suspicion& _suspicion;
};

}  // namespace Anticheat::Rules
