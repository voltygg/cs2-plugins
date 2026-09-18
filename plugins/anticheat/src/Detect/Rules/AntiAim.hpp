#pragma once

#include "Detect/CommandHistory.hpp"
#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"
#include "Detect/Suspicion.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace Anticheat::Rules
{

/** The command ring's depth, here because SlotData below holds one. Every other threshold this
 *  rule uses is private to the .cpp that reads it. */
inline constexpr size_t CommandHistorySize = 96;

/** A sustained spin or jitter is enough on its own, so it carries a whole unit of evidence.
 *  Shared because AntiAimMotion.cpp scores with it and the episode latch here reads it. */
inline constexpr float MotionWeight = 1.0f;

class AntiAim
{
public:
    /** The settings toggle and catalog entry this rule reports under. */
    static constexpr DetectionKind Kind = DetectionKind::AntiAim;

    explicit AntiAim(Suspicion& suspicion) : _suspicion(suspicion) {}

    void Reset();
    /** Also the spawn reset: a fresh pawn invalidates every in-flight command the same way. */
    void ClearSlot(int slot);

    /** Duplicates (same CmdNum) are dropped. */
    void OnCommand(int slot, const CmdSample& cmd);

    /**
     * The command the server simulates for @p serverTick. @p recentlyTeleported covers the spawn and
     * teleport grace, during which fake angles are indistinguishable from an engine-driven change.
     */
    void OnSimulated(int slot, int32_t cmdNum, int32_t serverTick, bool eligible, bool recentlyTeleported,
                     double nowSec);

    /** A correlated shot: arms the attack-return check for the command that fired it. */
    void OnWeaponFire(int slot, const ShotView& shot, double nowSec);

    /** Resolves an attack-return that is still waiting for the command after the shot. */
    void OnFrame(int slot, int32_t serverTick, bool eligible, double nowSec);

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
        CommandHistory<Command, CommandHistorySize> Commands;

        bool EpisodeReported = false;

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
        int32_t LastMotionCmdNum = -1;
    };

    static void ResetMotion(SlotData& data);
    void AddEvidence(int slot, SlotData& data, float weight, std::string_view reason, bool continuous, double nowSec);
    /** Sent back to back by the client, and simulated on consecutive ticks. */
    static bool IsAdjacent(const Command& older, const Command& newer);
    Command* Find(SlotData& data, int32_t cmdNum);
    /** Defined in AntiAimMotion.cpp. */
    void EvaluateMotion(int slot, SlotData& data, const Command& command, double nowSec);
    void EvaluatePendingShot(int slot, SlotData& data, int32_t currentTick, double nowSec);

    Suspicion& _suspicion;
    std::array<SlotData, MaxSlots> _slots{};
};

}  // namespace Anticheat::Rules
