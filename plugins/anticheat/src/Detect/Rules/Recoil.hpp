#pragma once

#include "Detect/CommandHistory.hpp"
#include "Detect/Evidence.hpp"
#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace Anticheat::Rules
{

/** How well one spray's view motion cancelled its recoil. */
struct SprayFit
{
    float Slope = 0.0f;          // view motion per unit of punch motion, positive when it cancels
    float ResidualDeg = 180.0f;  // RMS of what the slope leaves unexplained, per shot
    float PunchTravelDeg = 0.0f;
    bool Valid = false;
};

class Recoil
{
public:
    /** The settings toggle and catalog entry this rule reports under. */
    static constexpr DetectionKind Kind = DetectionKind::Recoil;

    static constexpr size_t CommandHistorySize = 256;

    void Reset();
    void OnSlotChanged(int slot);

    /** Every simulated command with the recoil punch the client predicted for it. */
    void OnCommand(int slot, const CmdSample& cmd);

    /** A correlated shot, at fire time. Closes the spray it does not belong to. */
    std::optional<Finding> OnShot(int slot, const ShotView& shot, double nowSec);

    /** Closes a spray that has gone quiet. */
    std::optional<Finding> OnFrame(int slot, int32_t serverTick, double nowSec);

    int Score(int slot, double nowSec) const;
    bool InSpray(int slot) const;

private:
    struct Command
    {
        int32_t CmdNum = 0;
        AimAngles View;
        AimAngles Punch;
        bool HasPunch = false;
    };

    struct Shot
    {
        int32_t CmdNum = 0;
        int32_t FireTick = -1;
        int ShotsFired = 0;
    };

    struct SlotData
    {
        CommandHistory<Command, CommandHistorySize> Commands;
        std::vector<Shot> Spray;
        std::string Weapon;
    };

    const Command* Find(const SlotData& data, int32_t cmdNum) const;
    /** The fit when the view reacts @p viewLag commands after the punch it cancels. */
    SprayFit Fit(const SlotData& data, int viewLag) const;
    std::optional<Finding> Finalize(int slot, SlotData& data, double nowSec);

    std::array<SlotData, MaxSlots> _slots{};
    std::array<LongEvidenceWindow, MaxSlots> _incidents{};
};

}  // namespace Anticheat::Rules
