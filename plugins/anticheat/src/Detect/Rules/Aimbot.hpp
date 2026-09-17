#pragma once

#include "Detect/CommandHistory.hpp"
#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"
#include "Detect/ShotHistory.hpp"
#include "Detect/Suspicion.hpp"

#include <array>
#include <optional>

namespace Anticheat::Rules
{

class Aimbot
{
public:
    /** The settings toggle and catalog entry this rule reports under. */
    static constexpr DetectionKind Kind = DetectionKind::Aimbot;

    static constexpr size_t CommandHistorySize = 128;

    Aimbot(const ShotHistory& shots, Suspicion& suspicion) : _shots(shots), _suspicion(suspicion) {}

    void Reset();
    void OnSlotChanged(int slot);

    /** Duplicates (same CmdNum) are dropped. */
    void OnCommand(int slot, const CmdSample& cmd);

    /** Stamps the command, and re-runs a pending evaluation that was waiting on it. */
    std::optional<Finding> OnSimulated(int slot, int32_t cmdNum, int32_t serverTick, const Vec3& eyePos, double nowSec);

    /** A damaging shot: the only kind this module judges. */
    std::optional<Finding> OnPlayerHurt(int attackerSlot, int victimSlot, ShotView& shot, double nowSec);

    /** Nudge so an evaluation waiting on a later command cannot hang forever. */
    std::optional<Finding> OnFrame(int slot, int32_t serverTick, bool eligible, double nowSec);


private:
    struct AimCommand
    {
        int32_t CmdNum = 0;
        int32_t ClientTick = 0;
        int32_t ServerTick = -1;
        AimAngles Angles;
        bool Simulated = false;
    };

    struct SlotData
    {
        CommandHistory<AimCommand, CommandHistorySize> Commands;
        int32_t PendingShot = 0;
        int VictimSlot = -1;
        bool Pending = false;
        int32_t LastCountedIncidentCommand = 0;
        bool HasCountedIncident = false;
    };

    AimCommand* Find(SlotData& data, int32_t cmdNum);
    void Evaluate(int slot, int32_t currentTick, double nowSec, std::optional<Finding>& out);
    void Count(int slot, SlotData& data, int32_t incidentCommand, double nowSec, bool snapReturn, float snap,
               float before, float after, std::optional<Finding>& out);

    const ShotHistory& _shots;
    Suspicion& _suspicion;
    std::array<SlotData, MaxSlots> _slots{};
};

}  // namespace Anticheat::Rules
