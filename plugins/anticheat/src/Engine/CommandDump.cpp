#include "Engine/CommandDump.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slots/Slot.hpp>
#include <format>
#include <string>

using VoltMod::Caller;
using VoltMod::IsValidSlot;
using VoltMod::Reply;
using VoltMod::Result;

namespace Args = VoltMod::Args;
namespace Log = VoltMod::Log;

namespace Anticheat
{

static constexpr int DefaultDumpTicks = 64;
static constexpr int MaxDumpTicks = 10000;

void CommandDump::Initialize()
{
    _remaining.BindReset(_rt.Slots);
    _subs.Add(_rt.Hooks.Movement.Before += [this](int slot, const VoltMod::PlayerInput& cmd) { OnCommand(slot, cmd); });

    _rt.Commands.Add("anticheat_dumpcmd")
        .Describe("Log raw usercmds for a slot.")
        .ConsoleOnly()
        .Run([this](Caller, Args::Int slot, Args::Opt<Args::Int> requested) -> Result<Reply> {
            if (!IsValidSlot(slot.Value))
                return Reply{std::format("anticheat_dumpcmd: {} is not a valid slot.", slot.Value)};

            const int ticks = requested.Value ? requested.Value->Value : DefaultDumpTicks;
            if (ticks < 1 || ticks > MaxDumpTicks)
                return Reply{std::format("anticheat_dumpcmd: ticks must be 1-{}.", MaxDumpTicks)};

            _remaining[slot.Value] = ticks;
            return Reply{std::format("Dumping {} usercmds for slot {}.", ticks, slot.Value)};
        });
}

void CommandDump::OnCommand(int slot, const VoltMod::PlayerInput& cmd)
{
    if (!cmd.Valid || !IsValidSlot(slot))
        return;
    int& remaining = _remaining[slot];
    if (remaining <= 0)
        return;
    --remaining;

    // Show whether the attack sample was present, capped, or absent.
    const int attackIndex = cmd.Attack1StartHistoryIndex;
    std::string attack = "none";
    if (attackIndex >= 0)
    {
        if (auto sample = cmd.SampleAt(attackIndex))
            attack = sample->HasViewAngles
                         ? std::format("[{}] pitch={:.2f} yaw={:.2f}", attackIndex, sample->ViewPitch, sample->ViewYaw)
                         : std::format("[{}] no angles", attackIndex);
        else
            attack = std::format("[{}] {}", attackIndex,
                                 attackIndex >= cmd.InputHistoryTotalCount ? "out of range" : "capped away");
    }

    float subtickPitch = 0.0f;
    float subtickYaw = 0.0f;
    for (int i = 0; i < cmd.SubtickMoveCount; ++i)
    {
        subtickPitch += cmd.SubtickMoves[i].PitchDelta;
        subtickYaw += cmd.SubtickMoves[i].YawDelta;
    }

    std::string punch = "none";
    if (const VoltMod::Pawn pawn = _rt.Entities.PawnOf(slot))
    {
        if (const auto services = pawn.AimPunchServices())
        {
            const QAngle base = services.BaseAngle();
            punch = std::format("({:.3f},{:.3f}) tick={}", base.x, base.y, services.BaseTick());
        }
    }

    Log::Info(
        "[AC dump s{}] cmd={} clientTick={} view=({:.2f},{:.2f},{:.2f}) mouse=({},{}) buttons={:#x}/{:#x} "
        "subticks={} (dPitch={:.3f} dYaw={:.3f}) history={}/{} attack1={} punch={}",
        slot, cmd.CommandNumber, cmd.ClientTick, cmd.ViewPitch, cmd.ViewYaw, cmd.ViewRoll, cmd.MouseDx, cmd.MouseDy,
        cmd.ButtonsHeld, cmd.ButtonsChanged, cmd.SubtickMoveCount, subtickPitch, subtickYaw,
        cmd.InputHistorySampleCount, cmd.InputHistoryTotalCount, attack, punch);
}

}  // namespace Anticheat
