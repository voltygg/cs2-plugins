#include "Detect/Rules/AntiAim.hpp"

#include "Detect/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

using Anticheat::Rules::AntiAim;

namespace Anticheat::Rules
{

// Per-command thresholds.
static constexpr float InvalidPitch = 89.01f;
static constexpr float InvalidRoll = 50.01f;
static constexpr float InvalidAnglesWeight = 0.02f;
static constexpr float InconsistentCommandWeight = 0.01f;
/** Worth less than the other per-command rules: it used to be discounted by decaying faster. */
static constexpr float HistoryMismatchWeight = 0.004f;
static constexpr float CommandYawMismatchAngle = 120.0f;
static constexpr int CommandMismatchSpacing = 4;
static constexpr float AttackReturnWeight = 0.05f;
static constexpr float MinimumAttackReturnAngle = 30.0f;
static constexpr float AttackReturnSurroundingAngle = 10.0f;
static constexpr float AttackReturnRatio = 5.0f;

void AntiAim::Reset()
{
    _slots = {};
}

void AntiAim::ClearSlot(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {};
}

void AntiAim::ResetMotion(SlotData& data)
{
    data.SpinSeconds = {};
    data.SpinBreakSeconds = {};
    data.JitterSeconds = 0.0f;
    data.JitterBreakSeconds = 0.0f;
    data.LastMotionCmdNum = -1;
    data.SpinActive = false;
    data.JitterActive = false;
}

void AntiAim::AddEvidence(int slot, SlotData& data, float weight, std::string_view reason, bool continuous,
                          double nowSec)
{
    if (data.EpisodeReported)
        return;

    if (!_suspicion.Add(slot,
                        {.Kind = Kind,
                         .Points = weight,
                         .HalfLifeSec = FadesOverSeconds,
                         .Reason = std::format("Anti-aim: {}.", reason)},
                        nowSec))
        return;

    // A continuous episode (spin, jitter, a stuck invalid angle) must not re-fire every command.
    data.EpisodeReported = continuous;
}

void AntiAim::OnCommand(int slot, const CmdSample& cmd)
{
    if (!InSlotRange(slot))
        return;

    Command captured;
    captured.CmdNum = cmd.CmdNum;
    captured.ClientTick = cmd.ClientTick;
    captured.Base = cmd.BaseAngles();
    captured.Roll = cmd.ViewRoll;
    captured.Attack = cmd.AttackStarted;
    captured.HasHistoryAngles = cmd.HasHistoryAngles;
    captured.HistoryYawDifference = cmd.MaxHistoryYawDelta;
    captured.Inconsistent = !cmd.BaseAnglesFinite || !Geometry::IsFinite(captured.Base) ||
                            !std::isfinite(captured.Roll) || cmd.AttackIndexInvalid || !cmd.HistoryAnglesFinite ||
                            !cmd.SubtickAnglesFinite || !std::isfinite(cmd.SubtickPitchDelta) ||
                            !std::isfinite(cmd.SubtickYawDelta);

    _slots[slot].Commands.Push(captured);
}

bool AntiAim::IsAdjacent(const Command& older, const Command& newer)
{
    return static_cast<int64_t>(newer.ClientTick) - older.ClientTick == 1 &&
           static_cast<int64_t>(newer.ServerTick) - older.ServerTick == 1;
}

AntiAim::Command* AntiAim::Find(SlotData& data, int32_t cmdNum)
{
    return data.Commands.FindIf([&](const Command& stored) { return stored.CmdNum == cmdNum && stored.Simulated; });
}

void AntiAim::OnSimulated(int slot, int32_t cmdNum, int32_t serverTick, bool eligible, bool recentlyTeleported,
                          double nowSec)
{
    if (!InSlotRange(slot))
        return;

    auto& data = _slots[slot];
    if (!eligible)
    {
        data.Commands.Clear();
        data.PendingShot = -1;
        data.PendingShotTick = -1;
        data.LastMismatchEvidenceCommand = -1;
        data.InvalidActive = false;
        data.InconsistencyActive = false;
        data.EpisodeReported = false;
        ResetMotion(data);
        return;
    }

    Command* found = data.Commands.Find(cmdNum);
    if (!found)
    {
        data.PendingShot = -1;
        data.PendingShotTick = -1;
        ResetMotion(data);
        return;
    }
    found->Simulated = true;
    found->ServerTick = serverTick;

    if (recentlyTeleported)
    {
        data.InvalidActive = data.InconsistencyActive = data.SpinActive = data.JitterActive = false;
        data.PendingShot = -1;
        data.PendingShotTick = -1;
        ResetMotion(data);
        return;
    }

    // A base view angle far from the angles the same command claims it fired along means one of the
    // two is fabricated. Fast legitimate flicks can do it too, so this only ever supports.
    const bool historyMismatch = !found->Attack && found->HasHistoryAngles &&
                                 std::isfinite(found->HistoryYawDifference) &&
                                 found->HistoryYawDifference >= CommandYawMismatchAngle;
    data.InconsistencyActive = found->Inconsistent || historyMismatch;
    if (found->Inconsistent)
    {
        AddEvidence(slot, data, InconsistentCommandWeight, "an inconsistent angle command", true, nowSec);
    }
    else if (historyMismatch &&
             (data.LastMismatchEvidenceCommand < 0 ||
              static_cast<int64_t>(found->CmdNum) - data.LastMismatchEvidenceCommand >= CommandMismatchSpacing))
    {
        data.LastMismatchEvidenceCommand = found->CmdNum;
        AddEvidence(slot, data, HistoryMismatchWeight, "a repeated base and input-history mismatch", true, nowSec);
    }

    const bool wasInvalid = data.InvalidActive;
    data.InvalidActive = Geometry::IsFinite(found->Base) && std::isfinite(found->Roll) &&
                         (std::abs(found->Base.Pitch) > InvalidPitch || std::abs(found->Roll) > InvalidRoll);
    if (data.InvalidActive && (!wasInvalid || !data.EpisodeReported))
        AddEvidence(slot, data, InvalidAnglesWeight, "invalid pitch or roll", true, nowSec);

    EvaluateMotion(slot, data, *found, nowSec);
    EvaluatePendingShot(slot, data, serverTick, nowSec);

    if (data.EpisodeReported && !data.InvalidActive && !data.InconsistencyActive && !data.SpinActive &&
        !data.JitterActive)
        data.EpisodeReported = false;
}

void AntiAim::EvaluatePendingShot(int slot, SlotData& data, int32_t currentTick, double nowSec)
{
    if (data.PendingShot < 0)
        return;
    if (data.PendingShot == std::numeric_limits<int32_t>::max())
    {
        data.PendingShot = -1;
        data.PendingShotTick = -1;
        return;
    }

    Command* previous = Find(data, data.PendingShot - 1);
    Command* shot = Find(data, data.PendingShot);
    Command* next = Find(data, data.PendingShot + 1);
    if (!next)
    {
        // Expire once the command after the shot can no longer arrive.
        if (static_cast<int64_t>(currentTick) - data.PendingShotTick > 1 ||
            (!data.Commands.Empty() && static_cast<int64_t>(data.Commands.Newest().CmdNum) - data.PendingShot > 1))
        {
            data.PendingShot = -1;
            data.PendingShotTick = -1;
        }
        return;
    }

    data.PendingShot = -1;
    data.PendingShotTick = -1;
    if (!previous || !shot || !IsAdjacent(*previous, *shot) || !IsAdjacent(*shot, *next))
        return;
    if (!Geometry::IsFinite(previous->Base) || !Geometry::IsFinite(shot->Base) || !Geometry::IsFinite(next->Base))
        return;

    const float surrounding = Geometry::AngularDistance(previous->Base, next->Base);
    const float snap = Geometry::AngularDistance(previous->Base, shot->Base);
    if (std::isfinite(surrounding) && std::isfinite(snap) && surrounding < AttackReturnSurroundingAngle &&
        snap > MinimumAttackReturnAngle && snap > surrounding * AttackReturnRatio)
        AddEvidence(slot, data, AttackReturnWeight, "one-command attack return", false, nowSec);
}

void AntiAim::OnWeaponFire(int slot, const ShotView& shot, double nowSec)
{
    if (!InSlotRange(slot) || shot.Slot != slot)
        return;

    auto& data = _slots[slot];
    const bool matched = data.Commands.FindIf([&](const Command& candidate) {
        return candidate.CmdNum == shot.CmdNum && candidate.Attack && candidate.Simulated &&
               candidate.ServerTick == shot.ServerTick;
    }) != nullptr;
    if (!matched)
        return;

    data.PendingShot = shot.CmdNum;
    data.PendingShotTick = shot.ServerTick;
    EvaluatePendingShot(slot, data, shot.FireTick, nowSec);
}

void AntiAim::OnFrame(int slot, int32_t serverTick, bool eligible, double nowSec)
{
    if (!InSlotRange(slot))
        return;

    auto& data = _slots[slot];
    if (data.PendingShot < 0 || static_cast<int64_t>(serverTick) - data.PendingShotTick <= 1)
        return;
    if (!eligible)
    {
        data.PendingShot = -1;
        data.PendingShotTick = -1;
        return;
    }
    EvaluatePendingShot(slot, data, serverTick, nowSec);
}

}  // namespace Anticheat::Rules
