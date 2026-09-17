#pragma once

// Reaction time from the moment an enemy walks into a resting crosshair to the shot that hits
// them. A human needs 150 ms and more; a triggerbot fires on the next command. SDK-free.

#include "Core/Evidence.hpp"
#include "Core/Finding.hpp"
#include "Core/LagEstimate.hpp"
#include "Core/Samples.hpp"
#include "Correlation/ShotCorrelatorCore.hpp"

#include <array>
#include <deque>
#include <optional>

namespace Anticheat
{

class TriggerbotCore
{
public:
    explicit TriggerbotCore(const ShotCorrelatorCore& shots) : _shots(shots) {}

    void Reset();
    void OnSlotChanged(int slot);

    /** The command the server simulates for @p serverTick. */
    void OnSimulated(int slot, int32_t serverTick, const AimAngles& angles, const Vec3& eyePos);

    /** Advance every target's on-crosshair run against the frame captured for @p serverTick. */
    void OnFrame(int slot, int32_t serverTick, bool aliveHuman, const LagEstimate& lag);

    /** Every ballistic fire event for @p slot, matched or not: it separates bursts. */
    void OnWeaponFire(int slot, int32_t fireTick);

    /** A shot that hurt someone, judged while the crosshair runs still describe the tick it fired. */
    std::optional<Finding> OnPlayerHurt(int slot, const ShotView& shot, double nowSec);

    int Score(int slot, double nowSec) const;

private:
    struct AimSample
    {
        int32_t ServerTick = -1;
        AimAngles Angles;
    };

    struct SlotData
    {
        SlotData() { ClearRuns(); }
        void ClearRuns();

        /** Since when the crosshair has rested on each target, per lag hypothesis; -1 when off. */
        std::array<std::array<int32_t, LagHypothesisCount>, MaxSlots> OnSince{};
        std::deque<AimSample> Aim;
        AimSample Pending;
        Vec3 PendingEye;
        bool PendingValid = false;
        int32_t LastFireTick = -1;
        int32_t PreviousFireTick = -1;
    };

    /** Largest angle the aim moved away from where it rested at @p sinceTick, up to @p untilTick. */
    static float AimTravel(const SlotData& data, int32_t sinceTick, int32_t untilTick);
    static bool OnTarget(const Vec3& eye, const AimAngles& angles, const PositionSample& target);

    const ShotCorrelatorCore& _shots;
    std::array<SlotData, MaxSlots> _slots{};
    std::array<LongEvidenceWindow, MaxSlots> _incidents{};
};

}  // namespace Anticheat
