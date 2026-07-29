#pragma once

// Looks only at commands that damaged an enemy, then asks how the aim arrived there: a human
// decelerates onto a target, an aimbot jumps onto it in one command and stops. SDK-free.

#include "Core/Evidence.hpp"
#include "Core/Finding.hpp"
#include "Core/Samples.hpp"
#include "Correlation/ShotCorrelatorCore.hpp"

#include <array>
#include <deque>
#include <optional>

namespace Anticheat
{

class AimbotCore
{
public:
    explicit AimbotCore(const ShotCorrelatorCore& shots) : _shots(shots) {}

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

    int IncidentCount(int slot) const;

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
        std::deque<AimCommand> Commands;
        LongEvidenceWindow Incidents;
        int32_t PendingShot = 0;
        int VictimSlot = -1;
        bool Pending = false;
        int32_t LastCountedIncidentCommand = 0;
        bool HasCountedIncident = false;
    };

    AimCommand* Find(SlotData& data, int32_t cmdNum);
    void Evaluate(int slot, int32_t currentTick, double nowSec, std::optional<Finding>& out);
    void Count(SlotData& data, int32_t incidentCommand, double nowSec, bool snapReturn, float snap, float before,
               float after, std::optional<Finding>& out);

    const ShotCorrelatorCore& _shots;
    std::array<SlotData, MaxSlots> _slots{};
};

}  // namespace Anticheat
