#pragma once

#include "Detect/Finding.hpp"
#include "Detect/ViewLag.hpp"
#include "Detect/Samples.hpp"
#include "Detect/ShotHistory.hpp"
#include "Detect/Suspicion.hpp"

#include <array>
#include <deque>
#include <optional>

namespace Anticheat::Rules
{

class Triggerbot
{
public:
    /** The settings toggle and catalog entry this rule reports under. */
    static constexpr DetectionKind Kind = DetectionKind::Triggerbot;

    Triggerbot(const ShotHistory& shots, Suspicion& suspicion) : _shots(shots), _suspicion(suspicion) {}

    void Reset();
    void OnSlotChanged(int slot);

    /** The command the server simulates for @p serverTick. */
    void OnSimulated(int slot, int32_t serverTick, const AimAngles& angles, const Vec3& eyePos);

    /** Advance every target's on-crosshair run against the frame captured for @p serverTick. */
    void OnFrame(int slot, int32_t serverTick, bool aliveHuman, const ViewLag& lag);

    /** Every ballistic fire event for @p slot, matched or not: it separates bursts. */
    void OnWeaponFire(int slot, int32_t fireTick);

    /** A shot that hurt someone, judged while the crosshair runs still describe the tick it fired. */
    std::optional<Finding> OnPlayerHurt(int slot, const ShotView& shot, double nowSec);


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

    const ShotHistory& _shots;
    Suspicion& _suspicion;
    std::array<SlotData, MaxSlots> _slots{};
};

}  // namespace Anticheat::Rules
