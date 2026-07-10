#include "CheatSimulator.hpp"

#include "../Detectors/Detection.hpp"
#include "../Managers.hpp"
#include "../PlayerMonitor.hpp"

#include <CS2Kit/Core/Slot.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <tier1/convar.h>

namespace Anticheat
{

using CS2Kit::Core::Engine;
using CS2Kit::Core::IsValidSlot;
namespace Log = CS2Kit::Utils::Log;

void CheatSimulator::Initialize()
{
    _sim.BindReset();

    _cmdSpin.emplace("anticheat_sim_spin", "Sim spinbot: anticheat_sim_spin <slot|steamid64> [degPerSec=720]",
                     [this](const CCommand& args) { Arm(args, Kind::Spin, 720.0f); });
    _cmdAimlock.emplace("anticheat_sim_aimlock", "Sim aimlock: anticheat_sim_aimlock <slot|steamid64> [snapDeg=45]",
                        [this](const CCommand& args) { Arm(args, Kind::Aimlock, 45.0f); });
    _cmdSilent.emplace("anticheat_sim_silent",
                       "Sim silent aim: anticheat_sim_silent <slot|steamid64> [divergenceDeg=15]",
                       [this](const CCommand& args) { Arm(args, Kind::Silent, 15.0f); });
    _cmdOff.emplace("anticheat_sim_off", "Stop simulating: anticheat_sim_off [slot|steamid64] (omit to clear all)",
                    [this](const CCommand& args) {
                        if (args.ArgC() < 2)
                        {
                            _sim.ResetAll();
                            return;
                        }
                        int slot = ResolveSlot(args.Arg(1));
                        if (IsValidSlot(slot))
                            _sim[slot] = {};
                        else
                            Log::Warn("'{}' is not a live slot or steamid64.", args.Arg(1));
                    });

    Log::Info("Cheat simulator ready; enable with anticheat.debug.simulator.enabled.");
}

bool CheatSimulator::Enabled() const
{
    return App().Config.Get().anticheat.debug.simulator.enabled;
}

int CheatSimulator::ResolveSlot(const char* arg)
{
    if (std::strlen(arg) > 10)  // too long to be a slot index; treat as a steamid64
    {
        int64_t steamId = std::strtoll(arg, nullptr, 10);
        auto* player = Engine().Players.GetPlayerBySteamId(steamId);
        return player ? player->GetSlot() : -1;
    }
    return std::atoi(arg);
}

void CheatSimulator::Arm(const CCommand& args, Kind kind, float defaultParam)
{
    if (!Enabled())
    {
        Log::Warn("Simulator disabled; set anticheat.debug.simulator.enabled and anticheat_reload.");
        return;
    }
    if (args.ArgC() < 2)
    {
        Log::Warn("Usage: {} <slot|steamid64> [param]", args.Arg(0));
        return;
    }

    int slot = ResolveSlot(args.Arg(1));
    if (!IsValidSlot(slot))
    {
        Log::Warn("'{}' is not a live slot or steamid64.", args.Arg(1));
        return;
    }

    // The filter rewrites live player commands, so it stays uninstalled until a sim is first armed;
    // a disabled simulator then costs nothing on the per-tick movement path.
    if (!_filtering)
    {
        Engine().MovementHook.ListenFilterCmd(
            [this](int filtered, CS2Kit::UserCmdView& cmd) { OnFilter(filtered, cmd); });
        _filtering = true;
    }

    float param = args.ArgC() > 2 ? std::strtof(args.Arg(2), nullptr) : defaultParam;
    auto& s = _sim[slot];
    s = {};
    s.kind = kind;
    s.param = param;
    s.holdTicks = kind == Kind::Aimlock ? 8 : 0;
    s.armed = kind == Kind::Aimlock;
    s.expireAt = NowSeconds() + 10.0;
    Log::Info("Simulating slot {} (param {:.1f}) for 10s.", slot, param);
}

void CheatSimulator::OnFilter(int slot, CS2Kit::UserCmdView& cmd)
{
    if (!Enabled() || !cmd.Valid || !IsValidSlot(slot))
        return;

    auto& s = _sim[slot];
    if (s.kind == Kind::Off)
        return;
    if (NowSeconds() > s.expireAt)
    {
        s.kind = Kind::Off;
        return;
    }

    switch (s.kind)
    {
    case Kind::Spin:
    {
        float step = s.param / TickRate;
        s.spinYaw = std::fmod(s.spinYaw + step, 360.0f);
        cmd.ViewYaw = s.spinYaw;
        cmd.SubtickMoveCount = 1;
        cmd.SubtickMoves[0] = {};
        cmd.SubtickMoves[0].YawDelta = step;  // the unwrapped truth the spin detector reads
        break;
    }
    case Kind::Aimlock:
        if (s.armed)  // first tick: snap to (current + snapDeg), then hold it still
        {
            s.lockYaw = cmd.ViewYaw + s.param;
            s.lockPitch = cmd.ViewPitch;
            s.armed = false;
        }
        cmd.ViewYaw = s.lockYaw;
        cmd.ViewPitch = s.lockPitch;
        cmd.SubtickMoveCount = 0;  // no motion during the lock, so the snap "settles"
        if (--s.holdTicks <= 0)
            s.kind = Kind::Off;
        break;
    case Kind::Silent:
        // Diverge the fired shot angle from the visible view; leave the visible view alone.
        cmd.Attack1StartHistoryIndex = 0;
        if (cmd.InputHistorySampleCount < 1)
            cmd.InputHistorySampleCount = 1;
        cmd.InputHistorySamples[0].HasViewAngles = true;
        cmd.InputHistorySamples[0].ViewYaw = cmd.ViewYaw + s.param;
        cmd.InputHistorySamples[0].ViewPitch = cmd.ViewPitch;
        cmd.MouseDx = 0;
        cmd.MouseDy = 0;
        break;
    case Kind::Off:
        break;
    }
}

}  // namespace Anticheat
