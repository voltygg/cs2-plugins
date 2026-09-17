#pragma once

#include "Detect/Evidence.hpp"
#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"
#include "Detect/ShotCorrelatorCore.hpp"

#include <array>
#include <deque>
#include <optional>

namespace Anticheat
{

class MouseCore
{
public:
    /** The settings toggle and catalog entry this core reports under. */
    static constexpr DetectionKind Kind = DetectionKind::MouseMismatch;

    explicit MouseCore(const ShotCorrelatorCore& shots) : _shots(shots) {}

    void Reset();
    void OnSlotChanged(int slot);

    /** The command the server simulates for @p serverTick, compared with the one before it. */
    std::optional<Finding> OnSimulated(int slot, const CmdSample& cmd, int32_t serverTick, bool justTeleported,
                                       double nowSec);

    /** True once the slot's mouse scale is known and its counts agree with its turns. */
    bool Calibrated(int slot) const;
    int Score(int slot, double nowSec) const;

private:
    /** Signed degrees per mouse count on one axis, learned from the player's own turns. */
    struct Axis
    {
        std::deque<float> Ratios;
        float Scale = 0.0f;
        bool Ready = false;

        void Learn(float ratio);
    };

    struct SlotData
    {
        CmdSample Last;
        int32_t LastTick = -1;
        bool HasLast = false;
        Axis Yaw;
        Axis Pitch;
    };

    /** Aim error against the nearest opponent's body, or nullopt with nobody to aim at. */
    std::optional<float> NearestOpponentError(int slot, int32_t serverTick, const Vec3& eye,
                                              const AimAngles& angles) const;

    const ShotCorrelatorCore& _shots;
    std::array<SlotData, MaxSlots> _slots{};
    std::array<LongEvidenceWindow, MaxSlots> _incidents{};
};

}  // namespace Anticheat
