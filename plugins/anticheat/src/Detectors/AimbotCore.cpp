#include "AimbotCore.hpp"

#include "Core/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

namespace Anticheat
{

namespace
{
constexpr size_t CommandHistorySize = 128;
constexpr int SnapWindowTicks = static_cast<int>(TickRate * 0.5f);  // 32
constexpr float MinimumDistance = 100.0f;
constexpr int DetectionThreshold = 4;
constexpr double EvidenceWindowSec = 600.0;  // 10 minutes

// Convergence: a large jump that lands far closer to the target than it started. The two branches
// trade snap size against how completely the error collapsed.
constexpr float WideSnapDeg = 10.0f;
constexpr float WideSnapErrorRatio = 0.2f;
constexpr float TightSnapDeg = 5.0f;
constexpr float TightSnapErrorRatio = 0.1f;

// Snap-return: one command steps far off the line its neighbours share, then steps back.
constexpr float SnapReturnSurroundingDeg = 10.0f;
constexpr float SnapReturnMinSnapDeg = 0.5f;
constexpr float SnapReturnRatio = 5.0f;
}  // namespace

void AimbotCore::Reset()
{
    _slots = {};
}

void AimbotCore::OnSlotChanged(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {};
}

void AimbotCore::OnCommand(int slot, const CmdSample& cmd)
{
    if (!InSlotRange(slot))
        return;

    const AimAngles angles = cmd.FiringAngles();
    if (!Geometry::IsFinite(angles))
        return;

    auto& commands = _slots[slot].Commands;
    if (std::any_of(commands.rbegin(), commands.rend(),
                    [&](const AimCommand& stored) { return stored.CmdNum == cmd.CmdNum; }))
        return;

    commands.push_back({.CmdNum = cmd.CmdNum, .ClientTick = cmd.ClientTick, .Angles = angles});
    while (commands.size() > CommandHistorySize)
        commands.pop_front();
}

AimbotCore::AimCommand* AimbotCore::Find(SlotData& data, int32_t cmdNum)
{
    auto found = std::find_if(data.Commands.begin(), data.Commands.end(),
                              [&](const AimCommand& command) { return command.CmdNum == cmdNum && command.Simulated; });
    return found == data.Commands.end() ? nullptr : &*found;
}

std::optional<Finding> AimbotCore::OnSimulated(int slot, int32_t cmdNum, int32_t serverTick, const Vec3& eyePos,
                                               double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot) || !Geometry::IsFinite(eyePos))
        return out;

    auto& data = _slots[slot];
    auto found = std::find_if(data.Commands.rbegin(), data.Commands.rend(),
                              [&](const AimCommand& stored) { return stored.CmdNum == cmdNum; });
    if (found == data.Commands.rend())
        return out;

    found->ServerTick = serverTick;
    found->Simulated = true;
    if (data.Pending)
        Evaluate(slot, serverTick, nowSec, out);
    return out;
}

std::optional<Finding> AimbotCore::OnFrame(int slot, int32_t serverTick, bool eligible, double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot) || !_slots[slot].Pending)
        return out;
    if (!eligible)
    {
        _slots[slot] = {};
        return out;
    }
    Evaluate(slot, serverTick, nowSec, out);
    return out;
}

std::optional<Finding> AimbotCore::OnPlayerHurt(int attackerSlot, int victimSlot, ShotView& shot, double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(attackerSlot) || !InSlotRange(victimSlot) || attackerSlot == victimSlot ||
        shot.Slot != attackerSlot || shot.AimbotConsumed)
        return out;

    shot.AimbotConsumed = true;
    auto& data = _slots[attackerSlot];
    if (data.Pending)
    {
        Evaluate(attackerSlot, shot.FireTick, nowSec, out);
        data.Pending = false;  // the previous shot never resolved; the new one takes its place
    }
    data.PendingShot = shot.CmdNum;
    data.VictimSlot = victimSlot;
    data.Pending = true;
    Evaluate(attackerSlot, shot.FireTick, nowSec, out);
    return out;
}

void AimbotCore::Evaluate(int slot, int32_t currentTick, double nowSec, std::optional<Finding>& out)
{
    auto& data = _slots[slot];
    if (!data.Pending)
        return;

    const auto clearPending = [&] {
        data.PendingShot = 0;
        data.VictimSlot = -1;
        data.Pending = false;
    };

    AimCommand* shot = Find(data, data.PendingShot);
    if (!shot || !InSlotRange(data.VictimSlot))
    {
        clearPending();
        return;
    }

    const PositionSample* shotTarget = _shots.FindPosition(shot->ServerTick, data.VictimSlot);
    const PositionSample* shotAttacker = _shots.FindPosition(shot->ServerTick, slot);
    if (!shotTarget || !shotAttacker)
    {
        // The frame for the shot's tick may still be captured this frame; only give up once past it.
        if (currentTick <= shot->ServerTick)
            return;
        clearPending();
        return;
    }
    if (shotTarget->Teleported || shotAttacker->Teleported ||
        !_shots.AreOpponents(shotAttacker->Team, shotTarget->Team))
    {
        clearPending();
        return;
    }

    const float distance = (shotTarget->Origin - shotAttacker->EyePos).Length();
    if (!std::isfinite(distance) || distance < MinimumDistance)
    {
        clearPending();
        return;
    }

    bool suspicious = false;
    bool snapReturn = false;
    float largestSnap = 0.0f;
    float bestBefore = 0.0f;
    float bestAfter = 0.0f;

    // Walk backwards along strictly adjacent commands: one lost command and the chain ends, because
    // a gap could hide the human motion that explains the jump.
    AimCommand* newer = shot;
    while (newer->CmdNum > std::numeric_limits<int32_t>::min())
    {
        AimCommand* older = Find(data, newer->CmdNum - 1);
        if (!older)
            break;
        const int64_t serverGap = static_cast<int64_t>(newer->ServerTick) - older->ServerTick;
        if (static_cast<int64_t>(newer->ClientTick) - older->ClientTick != 1 || serverGap < 0 || serverGap > 1)
            break;
        if (static_cast<int64_t>(shot->ServerTick) - older->ServerTick > SnapWindowTicks)
            break;

        const PositionSample* olderTarget = _shots.FindPosition(older->ServerTick, data.VictimSlot);
        const PositionSample* newerTarget = _shots.FindPosition(newer->ServerTick, data.VictimSlot);
        const PositionSample* olderAttacker = _shots.FindPosition(older->ServerTick, slot);
        const PositionSample* newerAttacker = _shots.FindPosition(newer->ServerTick, slot);
        if (!olderTarget || !newerTarget || !olderAttacker || !newerAttacker || olderTarget->Teleported ||
            newerTarget->Teleported || olderAttacker->Teleported || newerAttacker->Teleported)
            break;

        const float snap = Geometry::AngularDistance(older->Angles, newer->Angles);
        const float before = Geometry::NearestBodyAimError(olderAttacker->EyePos, older->Angles, olderTarget->Origin);
        const float after = Geometry::NearestBodyAimError(newerAttacker->EyePos, newer->Angles, newerTarget->Origin);
        if (!std::isfinite(snap) || !std::isfinite(before) || !std::isfinite(after))
            break;

        const bool converged = (snap > WideSnapDeg && after < before * WideSnapErrorRatio) ||
                               (snap > TightSnapDeg && after < before * TightSnapErrorRatio);
        const bool fresh = !data.HasCountedIncident || newer->CmdNum > data.LastCountedIncidentCommand;
        if (converged && fresh)
        {
            suspicious = true;
            if (snap > largestSnap)
            {
                largestSnap = snap;
                bestBefore = before;
                bestAfter = after;
            }
        }
        newer = older;
    }

    AimCommand* previous = Find(data, data.PendingShot - 1);
    AimCommand* next = Find(data, data.PendingShot + 1);
    if (!suspicious && !next)
    {
        // Snap-return needs the command after the shot; wait one tick for it before rejecting.
        if (static_cast<int64_t>(currentTick) - shot->ServerTick <= 1)
            return;
        clearPending();
        return;
    }

    if (previous && next && shot->CmdNum - previous->CmdNum == 1 && next->CmdNum - shot->CmdNum == 1 &&
        static_cast<int64_t>(shot->ClientTick) - previous->ClientTick == 1 &&
        static_cast<int64_t>(next->ClientTick) - shot->ClientTick == 1 &&
        static_cast<int64_t>(shot->ServerTick) - previous->ServerTick >= 0 &&
        static_cast<int64_t>(shot->ServerTick) - previous->ServerTick <= 1 &&
        static_cast<int64_t>(next->ServerTick) - shot->ServerTick >= 0 &&
        static_cast<int64_t>(next->ServerTick) - shot->ServerTick <= 1)
    {
        const float surrounding = Geometry::AngularDistance(previous->Angles, next->Angles);
        const float snap = Geometry::AngularDistance(previous->Angles, shot->Angles);
        const bool fresh = !data.HasCountedIncident || shot->CmdNum > data.LastCountedIncidentCommand;
        if (fresh && std::isfinite(surrounding) && std::isfinite(snap) && surrounding < SnapReturnSurroundingDeg &&
            snap > SnapReturnMinSnapDeg && snap > surrounding * SnapReturnRatio)
        {
            if (!suspicious)
            {
                snapReturn = true;
                largestSnap = snap;
            }
            suspicious = true;
        }
    }

    const int32_t incidentCommand = shot->CmdNum;
    clearPending();
    if (!suspicious)
        return;
    Count(data, incidentCommand, nowSec, snapReturn, largestSnap, bestBefore, bestAfter, out);
}

void AimbotCore::Count(SlotData& data, int32_t incidentCommand, double nowSec, bool snapReturn, float snap,
                       float before, float after, std::optional<Finding>& out)
{
    data.LastCountedIncidentCommand = incidentCommand;
    data.HasCountedIncident = true;

    while (!data.Incidents.empty() && nowSec - data.Incidents.front() >= EvidenceWindowSec)
        data.Incidents.pop_front();
    data.Incidents.push_back(nowSec);
    if (static_cast<int>(data.Incidents.size()) < DetectionThreshold)
        return;

    const size_t incidents = data.Incidents.size();
    data.Incidents.clear();
    if (out)
        return;

    out = Finding{
        .Kind = DetectionKind::Aimbot,
        .Evidence = snapReturn
                        ? std::format("{} snap-hit incidents; latest was a {:.2f} degree snap-return.", incidents, snap)
                        : std::format("{} snap-hit incidents; latest snap {:.2f} degrees, target error "
                                      "{:.2f} -> {:.2f} degrees.",
                                      incidents, snap, before, after)};
}

int AimbotCore::IncidentCount(int slot) const
{
    return InSlotRange(slot) ? static_cast<int>(_slots[slot].Incidents.size()) : 0;
}

}  // namespace Anticheat
