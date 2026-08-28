#pragma once

// Anti-aim / fake-angle detection: impossible pitch or roll, a base view angle that disagrees with
// the angles the client says it fired along, one-command attack returns, sustained spin, and exact
// repeating yaw jitter. Everything feeds one decaying score; motion patterns score it out instantly.
// Ingest, scoring and the per-command rules live here; spin/jitter in AntiAimMotionCore.cpp.

#include "Core/Finding.hpp"
#include "Core/Samples.hpp"

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string_view>

namespace Anticheat
{

/** In the header rather than a .cpp only because both TUs share them. */
namespace AntiAimTuning
{
inline constexpr size_t CommandHistorySize = 96;
inline constexpr float DetectionThreshold = 100.0f;
inline constexpr float ScoreDecayPerSecond = 2.0f;
inline constexpr float MismatchScoreDecayPerSecond = 5.0f;

// Per-command rules.
inline constexpr float InvalidPitch = 89.01f;
inline constexpr float InvalidRoll = 50.01f;
inline constexpr float InvalidAnglesWeight = 2.0f;
inline constexpr float InconsistentCommandWeight = 1.0f;
inline constexpr float HistoryMismatchWeight = 1.0f;
inline constexpr float CommandYawMismatchAngle = 120.0f;
inline constexpr int CommandMismatchSpacing = 4;
inline constexpr float AttackReturnWeight = 5.0f;
inline constexpr float MinimumAttackReturnAngle = 30.0f;
inline constexpr float AttackReturnSurroundingAngle = 10.0f;
inline constexpr float AttackReturnRatio = 5.0f;

// Motion rules (AntiAimMotionCore.cpp).
inline constexpr int MotionHistorySize = 20;
inline constexpr int SpinSamples = 16;
inline constexpr float MinimumSpinRate = 320.0f;
inline constexpr float MediumSpinRate = 1000.0f;
inline constexpr float FastSpinRate = 2200.0f;
inline constexpr float SlowSpinSeconds = 15.0f;
inline constexpr float MediumSpinSeconds = 10.0f;
inline constexpr float FastSpinSeconds = 10.0f;
inline constexpr float SpinBreakAllowance = 1.0f;
inline constexpr float SpinConsistency = 0.85f;
inline constexpr float JitterTolerance = 0.25f;
inline constexpr float MinimumJitterSpan = 10.0f;
inline constexpr float RequiredJitterSeconds = 10.0f;
}  // namespace AntiAimTuning

class AntiAimCore
{
public:
    void Reset();
    /** Also the spawn reset: a fresh pawn invalidates every in-flight command the same way. */
    void OnSlotChanged(int slot);

    /** Duplicates (same CmdNum) are dropped. */
    void OnCommand(int slot, const CmdSample& cmd);

    /**
     * The command the server simulates for @p serverTick. @p justTeleported covers the spawn and
     * teleport grace, during which fake angles are indistinguishable from an engine-driven change.
     */
    std::optional<Finding> OnSimulated(int slot, int32_t cmdNum, int32_t serverTick, bool eligible, bool justTeleported,
                                       double nowSec);

    /** A correlated shot: arms the attack-return check for the command that fired it. */
    std::optional<Finding> OnWeaponFire(int slot, const ShotView& shot, double nowSec);

    /** Resolves an attack-return that is still waiting for the command after the shot. */
    std::optional<Finding> OnFrame(int slot, int32_t serverTick, bool eligible, double nowSec);

    float Score(int slot) const;

private:
    struct Command
    {
        int32_t CmdNum = 0;
        int32_t ClientTick = 0;
        int32_t ServerTick = -1;
        AimAngles Base;
        float Roll = 0.0f;
        float HistoryYawDifference = 0.0f;
        /** Non-finite base, history or subtick angles, or an attack index the client never sent.
         *  All four weigh the same and read the same in the evidence, so they are one flag. */
        bool Inconsistent = false;
        bool Attack = false;
        bool HasHistoryAngles = false;
        bool Simulated = false;
    };

    struct SlotData
    {
        std::deque<Command> Commands;

        float Score = 0.0f;
        float MismatchScore = 0.0f;
        double ScoreTime = 0.0;
        bool SuppressContinuous = false;

        bool InvalidActive = false;
        bool InconsistencyActive = false;
        bool SpinActive = false;
        bool JitterActive = false;

        int32_t LastMismatchEvidenceCommand = -1;
        int32_t PendingShot = -1;
        int32_t PendingShotTick = -1;

        // Motion episodes, one entry per spin tier.
        std::array<float, 3> SpinSeconds{};
        std::array<float, 3> SpinBreakSeconds{};
        float JitterSeconds = 0.0f;
        float JitterBreakSeconds = 0.0f;
        int32_t LastMotionServerTick = -1;
    };

    static void ResetMotion(SlotData& data);
    static void ApplyDecay(SlotData& data, double nowSec);
    void AddEvidence(SlotData& data, float weight, std::string_view reason, bool continuous, bool mismatch,
                     double nowSec, std::optional<Finding>& out);
    Command* Find(SlotData& data, int32_t cmdNum);
    /** Defined in AntiAimMotionCore.cpp. */
    void EvaluateMotion(SlotData& data, const Command& command, double nowSec, std::optional<Finding>& out);
    void EvaluatePendingShot(SlotData& data, int32_t currentTick, double nowSec, std::optional<Finding>& out);

    std::array<SlotData, MaxSlots> _slots{};
};

}  // namespace Anticheat
