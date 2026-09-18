#pragma once

#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"
#include "Detect/ShotHistory.hpp"
#include "Detect/Suspicion.hpp"
#include "Detect/ViewLag.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>

namespace Anticheat::Rules
{

/** The frame each lag hypothesis looks back at, resolved once per tick rather than once per
 *  target and body point: the lookup walks the frame ring. */
struct LagFrames
{
    std::array<const PositionFrame*, LagHypothesisCount> Frames{};
    int First = 0;
    int Last = -1;  ///< below First when no estimate was usable
};

inline LagFrames ResolveLagFrames(const ShotHistory& shots, int32_t serverTick, const ViewLag& lag)
{
    LagFrames resolved;
    if (!lag.Valid)
        return resolved;

    resolved.First = std::max(0, lag.Ticks - LagSearchRadius);
    resolved.Last = lag.Ticks + LagSearchRadius;
    for (int lagTicks = resolved.First; lagTicks <= resolved.Last; ++lagTicks)
        resolved.Frames[lagTicks - resolved.First] = shots.FindFrame(serverTick - lagTicks);
    return resolved;
}

/** The frame @p lagTicks back; a hypothesis may outlive the estimate that opened it, so a lag
 *  outside the resolved window is looked up rather than read as missing. */
inline const PositionFrame* FrameForLag(const ShotHistory& shots, int32_t serverTick, const LagFrames& resolved,
                                        int lagTicks)
{
    if (lagTicks >= resolved.First && lagTicks <= resolved.Last)
        return resolved.Frames[lagTicks - resolved.First];
    return shots.FindFrame(serverTick - lagTicks);
}

class Aimlock
{
public:
    /** The settings toggle and catalog entry this rule reports under. */
    static constexpr DetectionKind Kind = DetectionKind::Aimlock;

    Aimlock(const ShotHistory& shots, Suspicion& suspicion) : _shots(shots), _suspicion(suspicion) {}

    void Reset();
    void ClearSlot(int slot);

    /** The one command the server will actually simulate for @p serverTick. */
    void OnSimulated(int slot, int32_t serverTick, const AimAngles& angles, const Vec3& eyePos);

    /** Advance or close the tracking episode for @p slot against the frame just captured. */
    void OnFrame(int slot, int32_t serverTick, bool aliveHuman, const ViewLag& lag, double nowSec);

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
        std::array<Hypothesis, LagHypothesisCount> Hypotheses{};
        int TargetSlot = -1;
        int BodyPoint = -1;
        int32_t StartServerTick = -1;
        int32_t LastServerTick = -1;
        int HypothesisCount = 0;
        int Samples = 0;
    };

    /** Dropped whenever the player stops being trackable (death, ineligibility). Accumulated
     *  evidence lives on the score instead, so dying between episodes does not clear it. */
    struct SlotData
    {
        Sample Pending;
        Track Current;
        int LockedTarget = -1;
        int LockedBodyPoint = -1;
        int32_t OffTargetSince = -1;
        int32_t LastProcessedTick = -1;
        bool Locked = false;
    };

    void Evaluate(int slot, SlotData& data, const Sample& sample, const ViewLag& lag, double nowSec);
    bool StillOnTarget(const Sample& sample, const PositionFrame& frame, const LagFrames& lagFrames, int slot,
                       int targetSlot, int bodyPoint) const;
    void StartTrack(int slot, SlotData& data, const Sample& sample, const LagFrames& lagFrames);
    void Count(int slot, SlotData& data, const Hypothesis& hypothesis, double nowSec);

    const ShotHistory& _shots;
    Suspicion& _suspicion;
    std::array<SlotData, MaxSlots> _slots{};
};

}  // namespace Anticheat::Rules
