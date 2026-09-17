#include "Detect/Rules/AntiAim.hpp"

#include "Detect/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

using Anticheat::Rules::AntiAim;
using Anticheat::Rules::AntiAimTuning::AttackReturnRatio;
using Anticheat::Rules::AntiAimTuning::AttackReturnSurroundingAngle;
using Anticheat::Rules::AntiAimTuning::AttackReturnWeight;
using Anticheat::Rules::AntiAimTuning::CommandHistorySize;
using Anticheat::Rules::AntiAimTuning::CommandMismatchSpacing;
using Anticheat::Rules::AntiAimTuning::CommandYawMismatchAngle;
using Anticheat::Rules::AntiAimTuning::DetectionThreshold;
using Anticheat::Rules::AntiAimTuning::HistoryMismatchWeight;
using Anticheat::Rules::AntiAimTuning::InconsistentCommandWeight;
using Anticheat::Rules::AntiAimTuning::InvalidAnglesWeight;
using Anticheat::Rules::AntiAimTuning::InvalidPitch;
using Anticheat::Rules::AntiAimTuning::InvalidRoll;
using Anticheat::Rules::AntiAimTuning::MinimumAttackReturnAngle;
using Anticheat::Rules::AntiAimTuning::MismatchScoreDecayPerSecond;
using Anticheat::Rules::AntiAimTuning::ScoreDecayPerSecond;

namespace Anticheat::Rules
{

void AntiAim::Reset()
{
    _slots = {};
}

void AntiAim::OnSlotChanged(int slot)
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

void AntiAim::ApplyDecay(SlotData& data, double nowSec)
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

void AntiAim::AddEvidence(SlotData& data, float weight, std::string_view reason, bool continuous, bool mismatch,
                              double nowSec, std::optional<Finding>& out)
{
    if (data.EpisodeReported)
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

AntiAim::Command* AntiAim::Find(SlotData& data, int32_t cmdNum)
{
    return data.Commands.FindIf([&](const Command& stored) { return stored.CmdNum == cmdNum && stored.Simulated; });
}

std::optional<Finding> AntiAim::OnSimulated(int slot, int32_t cmdNum, int32_t serverTick, bool eligible,
                                                bool recentlyTeleported, double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot))
        return out;

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
        return out;
    }

    ApplyDecay(data, nowSec);
    Command* found = data.Commands.Find(cmdNum);
    if (!found)
    {
        data.PendingShot = -1;
        data.PendingShotTick = -1;
        ResetMotion(data);
        return out;
    }
    found->Simulated = true;
    found->ServerTick = serverTick;

    if (recentlyTeleported)
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
    if (data.InvalidActive && (!wasInvalid || !data.EpisodeReported))
        AddEvidence(data, InvalidAnglesWeight, "invalid pitch or roll", true, false, nowSec, out);

    EvaluateMotion(data, *found, nowSec, out);
    EvaluatePendingShot(data, serverTick, nowSec, out);

    if (data.EpisodeReported && !data.InvalidActive && !data.InconsistencyActive && !data.SpinActive &&
        !data.JitterActive)
        data.EpisodeReported = false;
    return out;
}

void AntiAim::EvaluatePendingShot(SlotData& data, int32_t currentTick, double nowSec, std::optional<Finding>& out)
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

std::optional<Finding> AntiAim::OnWeaponFire(int slot, const ShotView& shot, double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot) || shot.Slot != slot)
        return out;

    auto& data = _slots[slot];
    const bool matched = data.Commands.FindIf([&](const Command& candidate) {
        return candidate.CmdNum == shot.CmdNum && candidate.Attack && candidate.Simulated &&
               candidate.ServerTick == shot.ServerTick;
    }) != nullptr;
    if (!matched)
        return out;

    data.PendingShot = shot.CmdNum;
    data.PendingShotTick = shot.ServerTick;
    EvaluatePendingShot(data, shot.FireTick, nowSec, out);
    return out;
}

std::optional<Finding> AntiAim::OnFrame(int slot, int32_t serverTick, bool eligible, double nowSec)
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

float AntiAim::Score(int slot) const
{
    if (!InSlotRange(slot))
        return 0.0f;
    return _slots[slot].Score + _slots[slot].MismatchScore;
}

}  // namespace Anticheat::Rules
