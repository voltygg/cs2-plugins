#pragma once

// Sustained, unnaturally precise tracking (including through walls): the aim stays inside the
// target's own angular width for 1.5 seconds while the target moves enough that a human would have
// drifted off it. SDK-free.

#include "Core/Evidence.hpp"
#include "Core/Finding.hpp"
#include "Core/Samples.hpp"
#include "Correlation/ShotCorrelatorCore.hpp"

#include <array>
#include <optional>

namespace Anticheat
{

/** How far in the past the world the client aimed at actually is, in server ticks. */
struct LagEstimate
{
    int Ticks = 0;
    float RoundTripMs = 0.0f;
    float InterpTicks = 0.0f;
    bool Valid = false;
};

/**
 * The snapshot travelled to the client before this command travelled back, so the round trip plus
 * the interpolation delay is the age of what the player saw. Invalid - and therefore never evidence
 * - for absurd RTT or cl_interp_ratio values.
 */
LagEstimate EstimateVisualLag(float rttSeconds, float interpRatio);

class AimlockCore
{
public:
    explicit AimlockCore(const ShotCorrelatorCore& shots) : _shots(shots) {}

    void Reset();
    void OnSlotChanged(int slot);

    /** The one command the server will actually simulate for @p serverTick. */
    void OnSimulated(int slot, int32_t serverTick, const AimAngles& angles, const Vec3& eyePos);

    /** Advance or close the tracking episode for @p slot against the frame just captured. */
    std::optional<Finding> OnFrame(int slot, int32_t serverTick, bool aliveHuman, const LagEstimate& lag,
                                   double nowSec);

    int IncidentCount(int slot) const;
    bool IsTracking(int slot) const;

private:
    struct Sample
    {
        int32_t ServerTick = -1;
        AimAngles Angles;
        Vec3 EyePos;
        bool Valid = false;
    };

    /** One "the client saw the world N ticks ago" theory, scored across the episode. */
    struct Hypothesis
    {
        AimAngles StartBearing;
        float MaxTargetDisplacement = 0.0f;
        float RequiredTargetDisplacement = 0.0f;
        int LagTicks = 0;
        int OnTargetSamples = 0;
        bool Valid = false;
    };

    struct Track
    {
        std::array<Hypothesis, 5> Hypotheses{};
        int TargetSlot = -1;
        int BodyPoint = -1;
        int32_t StartServerTick = -1;
        int32_t LastServerTick = -1;
        int HypothesisCount = 0;
        int Samples = 0;
    };

    /** Dropped whenever the player stops being trackable (death, ineligibility). Accumulated
     *  evidence deliberately lives outside it, in @ref _incidents. */
    struct SlotData
    {
        Sample Pending;
        Track Current;
        int LatchedTarget = -1;
        int LatchedBodyPoint = -1;
        int32_t BreakStartTick = -1;
        int32_t LastProcessedTick = -1;
        bool Latched = false;
    };

    void Evaluate(int slot, SlotData& data, const Sample& sample, const LagEstimate& lag, double nowSec,
                  std::optional<Finding>& out);
    void StartTrack(int slot, SlotData& data, const Sample& sample, const LagEstimate& lag);
    void Count(int slot, SlotData& data, const Hypothesis& hypothesis, double nowSec, std::optional<Finding>& out);

    const ShotCorrelatorCore& _shots;
    std::array<SlotData, MaxSlots> _slots{};
    /** Episodes inside the evidence window. Survives death and respawn, so a cheat that dies
     *  between episodes does not reset its own count. */
    std::array<LongEvidenceWindow, MaxSlots> _incidents{};
};

}  // namespace Anticheat
