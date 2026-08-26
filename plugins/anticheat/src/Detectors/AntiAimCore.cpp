#include "AntiAimCore.hpp"

#include "Core/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

using Anticheat::AntiAimCore;
using Anticheat::AntiAimTuning::AttackReturnRatio;
using Anticheat::AntiAimTuning::AttackReturnSurroundingAngle;
using Anticheat::AntiAimTuning::AttackReturnWeight;
using Anticheat::AntiAimTuning::CommandHistorySize;
using Anticheat::AntiAimTuning::CommandMismatchSpacing;
using Anticheat::AntiAimTuning::CommandYawMismatchAngle;
using Anticheat::AntiAimTuning::DetectionThreshold;
using Anticheat::AntiAimTuning::HistoryMismatchWeight;
using Anticheat::AntiAimTuning::InconsistentCommandWeight;
using Anticheat::AntiAimTuning::InvalidAnglesWeight;
using Anticheat::AntiAimTuning::InvalidPitch;
using Anticheat::AntiAimTuning::InvalidRoll;
using Anticheat::AntiAimTuning::MinimumAttackReturnAngle;
using Anticheat::AntiAimTuning::MismatchScoreDecayPerSecond;
using Anticheat::AntiAimTuning::ScoreDecayPerSecond;

namespace Anticheat
{

void AntiAimCore::Reset()
{
    _slots = {};
}

void AntiAimCore::OnSlotChanged(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {};
}

void AntiAimCore::ResetMotion(SlotData& data)
{
    data.SpinSeconds = {};
    data.SpinBreakSeconds = {};
    data.JitterSeconds = 0.0f;
    data.JitterBreakSeconds = 0.0f;
    data.LastMotionServerTick = -1;
    data.SpinActive = false;
    data.JitterActive = false;
}

void AntiAimCore::ApplyDecay(SlotData& data, double nowSec)
{
    if (data.ScoreTime == 0.0)
    {
        data.ScoreTime = nowSec;
        return;
    }
    const float elapsed = static_cast<float>(nowSec - data.ScoreTime);
    data.ScoreTime = nowSec;
    if (elapsed <= 0.0f || (data.Score <= 0.0f && data.MismatchScore <= 0.0f))
        return;
    data.Score = std::max(0.0f, data.Score - elapsed * ScoreDecayPerSecond);
    data.MismatchScore = std::max(0.0f, data.MismatchScore - elapsed * MismatchScoreDecayPerSecond);
}

void AntiAimCore::AddEvidence(SlotData& data, float weight, const char* reason, bool continuous, bool mismatch,
                              double nowSec, std::optional<Finding>& out)
{
    if (data.SuppressContinuous)
        return;
    ApplyDecay(data, nowSec);
    (mismatch ? data.MismatchScore : data.Score) += weight;

    const float total = data.Score + data.MismatchScore;
    if (total < DetectionThreshold)
        return;

    if (!out)
        out = Finding{.Kind = DetectionKind::AntiAim,
                      .Evidence = std::format("{} added {:.1f} points and reached {:.1f}/{:.0f} evidence.", reason,
                                              weight, total, DetectionThreshold)};
    data.Score = 0.0f;
    data.MismatchScore = 0.0f;
    data.ScoreTime = nowSec;
    // A continuous episode (spin, jitter, a stuck invalid angle) must not re-fire every command.
    data.SuppressContinuous = continuous;
}

void AntiAimCore::OnCommand(int slot, const CmdSample& cmd)
{
    if (!InSlotRange(slot))
        return;

    auto& commands = _slots[slot].Commands;
    if (std::any_of(commands.rbegin(), commands.rend(),
                    [&](const Command& stored) { return stored.CmdNum == cmd.CmdNum; }))
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

    commands.push_back(captured);
    while (commands.size() > CommandHistorySize)
        commands.pop_front();
}

AntiAimCore::Command* AntiAimCore::Find(SlotData& data, int32_t cmdNum)
{
    auto found = std::find_if(data.Commands.rbegin(), data.Commands.rend(),
                              [&](const Command& stored) { return stored.CmdNum == cmdNum && stored.Simulated; });
    return found == data.Commands.rend() ? nullptr : &*found;
}

std::optional<Finding> AntiAimCore::OnSimulated(int slot, int32_t cmdNum, int32_t serverTick, bool eligible,
                                                bool justTeleported, double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot))
        return out;

    auto& data = _slots[slot];
    if (!eligible)
    {
        data.Commands.clear();
        data.PendingShot = -1;
        data.PendingShotTick = -1;
        data.LastMismatchEvidenceCommand = -1;
        data.InvalidActive = false;
        data.InconsistencyActive = false;
        data.SuppressContinuous = false;
        ResetMotion(data);
        return out;
    }

    ApplyDecay(data, nowSec);
    auto found = std::find_if(data.Commands.rbegin(), data.Commands.rend(),
                              [&](const Command& stored) { return stored.CmdNum == cmdNum; });
    if (found == data.Commands.rend())
    {
        data.PendingShot = -1;
        data.PendingShotTick = -1;
        ResetMotion(data);
        return out;
    }
    found->Simulated = true;
    found->ServerTick = serverTick;

    if (justTeleported)
    {
        data.InvalidActive = data.InconsistencyActive = data.SpinActive = data.JitterActive = false;
        data.PendingShot = -1;
        data.PendingShotTick = -1;
        ResetMotion(data);
        return out;
    }

    // A base view angle far from the angles the same command claims it fired along means one of the
    // two is fabricated. Fast legitimate flicks can do it too, so this only ever supports.
    const bool historyMismatch = !found->Attack && found->HasHistoryAngles &&
                                 std::isfinite(found->HistoryYawDifference) &&
                                 found->HistoryYawDifference >= CommandYawMismatchAngle;
    data.InconsistencyActive = found->Inconsistent || historyMismatch;
    if (found->Inconsistent)
    {
        AddEvidence(data, InconsistentCommandWeight, "an inconsistent angle command", true, false, nowSec, out);
    }
    else if (historyMismatch &&
             (data.LastMismatchEvidenceCommand < 0 ||
              static_cast<int64_t>(found->CmdNum) - data.LastMismatchEvidenceCommand >= CommandMismatchSpacing))
    {
        data.LastMismatchEvidenceCommand = found->CmdNum;
        AddEvidence(data, HistoryMismatchWeight, "a repeated base and input-history mismatch", true, true, nowSec, out);
    }

    const bool wasInvalid = data.InvalidActive;
    data.InvalidActive = Geometry::IsFinite(found->Base) && std::isfinite(found->Roll) &&
                         (std::abs(found->Base.Pitch) > InvalidPitch || std::abs(found->Roll) > InvalidRoll);
    if (data.InvalidActive && (!wasInvalid || !data.SuppressContinuous))
        AddEvidence(data, InvalidAnglesWeight, "invalid pitch or roll", true, false, nowSec, out);

    EvaluateMotion(data, *found, nowSec, out);
    EvaluatePendingShot(data, serverTick, nowSec, out);

    if (data.SuppressContinuous && !data.InvalidActive && !data.InconsistencyActive && !data.SpinActive &&
        !data.JitterActive)
        data.SuppressContinuous = false;
    return out;
}

void AntiAimCore::EvaluatePendingShot(SlotData& data, int32_t currentTick, double nowSec, std::optional<Finding>& out)
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
            (!data.Commands.empty() && static_cast<int64_t>(data.Commands.back().CmdNum) - data.PendingShot > 1))
        {
            data.PendingShot = -1;
            data.PendingShotTick = -1;
        }
        return;
    }

    data.PendingShot = -1;
    data.PendingShotTick = -1;
    if (!previous || !shot || !Geometry::IsFinite(previous->Base) || !Geometry::IsFinite(shot->Base) ||
        !Geometry::IsFinite(next->Base) || static_cast<int64_t>(shot->ClientTick) - previous->ClientTick != 1 ||
        static_cast<int64_t>(next->ClientTick) - shot->ClientTick != 1 ||
        static_cast<int64_t>(shot->ServerTick) - previous->ServerTick != 1 ||
        static_cast<int64_t>(next->ServerTick) - shot->ServerTick != 1)
        return;

    const float surrounding = Geometry::AngularDistance(previous->Base, next->Base);
    const float snap = Geometry::AngularDistance(previous->Base, shot->Base);
    if (std::isfinite(surrounding) && std::isfinite(snap) && surrounding < AttackReturnSurroundingAngle &&
        snap > MinimumAttackReturnAngle && snap > surrounding * AttackReturnRatio)
        AddEvidence(data, AttackReturnWeight, "one-command attack return", false, false, nowSec, out);
}

std::optional<Finding> AntiAimCore::OnWeaponFire(int slot, const ShotView& shot, double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot) || shot.Slot != slot)
        return out;

    auto& data = _slots[slot];
    const bool matched = std::any_of(data.Commands.rbegin(), data.Commands.rend(), [&](const Command& candidate) {
        return candidate.CmdNum == shot.CmdNum && candidate.Attack && candidate.Simulated &&
               candidate.ServerTick == shot.ServerTick;
    });
    if (!matched)
        return out;

    data.PendingShot = shot.CmdNum;
    data.PendingShotTick = shot.ServerTick;
    EvaluatePendingShot(data, shot.FireTick, nowSec, out);
    return out;
}

std::optional<Finding> AntiAimCore::OnFrame(int slot, int32_t serverTick, bool eligible, double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot))
        return out;

    auto& data = _slots[slot];
    if (data.PendingShot < 0 || static_cast<int64_t>(serverTick) - data.PendingShotTick <= 1)
        return out;
    if (!eligible)
    {
        data.PendingShot = -1;
        data.PendingShotTick = -1;
        return out;
    }
    EvaluatePendingShot(data, serverTick, nowSec, out);
    return out;
}

float AntiAimCore::Score(int slot) const
{
    if (!InSlotRange(slot))
        return 0.0f;
    return _slots[slot].Score + _slots[slot].MismatchScore;
}

}  // namespace Anticheat
