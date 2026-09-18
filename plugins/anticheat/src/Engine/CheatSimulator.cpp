#include "Engine/CheatSimulator.hpp"

#include "Detect/Geometry.hpp"
#include "Detect/Samples.hpp"
#include "Detect/ShotHistory.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slots/Slot.hpp>
#include <VoltMod/Core/Time/Time.hpp>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <format>
#include <mathlib/vector.h>
#include <string>
#include <tier1/convar.h>

namespace Anticheat
{

using VoltMod::IsValidSlot;
using VoltMod::Time;
namespace Log = VoltMod::Log;

static constexpr double SimulationSeconds = 10.0;
/** Three is the middle jitter period AntiAim looks for. */
static constexpr int JitterPeriod = 3;
/** Well past AntiAim's 50.01 degree roll ceiling. */
static constexpr float BadRoll = 60.0f;
/** Chest height, so the lock reads as a body point rather than a graze. */
static constexpr float LockHeight = 46.0f;
/** A rename every quarter second: the pace of an animated clan tag. */
static constexpr int RenameEveryCommands = 16;

void CheatSimulator::Initialize()
{
    _sim.BindReset(_rt.Slots);

    if (!Enabled())
    {
        Log::Info("Cheat simulator idle; enable anticheat.debug.simulator and reload the plugin to enable it.");
        return;
    }

    struct Pattern
    {
        const char* Name;
        const char* Help;
        Kind Simulated;
        float DefaultParam;
    };
    static constexpr Pattern patterns[] = {
        {"anticheat_sim_spin", "Sim spinbot: anticheat_sim_spin <slot|steamid64> [degPerSec=720]", Kind::Spin, 720.0f},
        {"anticheat_sim_jitter", "Sim yaw jitter: anticheat_sim_jitter <slot|steamid64> [stepDeg=20]", Kind::Jitter,
         20.0f},
        {"anticheat_sim_badangles",
         "Sim impossible pitch and roll: anticheat_sim_badangles <slot|steamid64> [pitch=89.5]", Kind::BadAngles,
         89.5f},
        {"anticheat_sim_aimlock", "Sim locking onto the nearest opponent: anticheat_sim_aimlock <slot|steamid64>",
         Kind::Aimlock, 0.0f},
        {"anticheat_sim_mismatch",
         "Sim input-history angles diverging from the view: anticheat_sim_mismatch <slot|steamid64> [deg=130]",
         Kind::Mismatch, 130.0f},
        {"anticheat_sim_nomouse", "Sim view turns without mouse counts: anticheat_sim_nomouse <slot|steamid64>",
         Kind::NoMouse, 0.0f},
        {"anticheat_sim_names", "Sim a name changer: anticheat_sim_names <slot|steamid64>", Kind::Names, 0.0f},
    };

    for (const Pattern& pattern : patterns)
        _commands.emplace_back(pattern.Name, pattern.Help, [this, pattern](const CCommand& args) {
            BeginPattern(args, pattern.Simulated, pattern.DefaultParam);
        });

    _commands.emplace_back("anticheat_sim_off",
                           "Stop simulating: anticheat_sim_off [slot|steamid64] (omit to clear all)",
                           [this](const CCommand& args) {
                               if (args.ArgC() < 2)
                               {
                                   _sim.ResetAll();
                                   return;
                               }
                               const int slot = ResolveSlot(args.Arg(1));
                               if (IsValidSlot(slot))
                                   _sim[slot] = {};
                               else
                                   Log::Warn("'{}' is not a live slot or steamid64.", args.Arg(1));
                           });

    Log::Info("Cheat simulator ready (anticheat_sim_*).");
}

bool CheatSimulator::Enabled() const
{
    return _config.Get().anticheat.debug.simulator;
}

int CheatSimulator::ResolveSlot(std::string_view arg)
{
    const auto number = VoltMod::ParseInt64(arg);
    if (!number)
        return -1;

    if (arg.size() > 10)  // too long to be a slot index; treat as a steamid64
    {
        auto* player = _rt.Players.BySteamId(*number);
        return player ? player->Slot() : -1;
    }
    return static_cast<int>(*number);
}

void CheatSimulator::BeginPattern(const CCommand& args, Kind kind, float defaultParam)
{
    if (!Enabled())
    {
        Log::Warn("Simulator disabled; set anticheat.debug.simulator and reload the plugin.");
        return;
    }
    if (args.ArgC() < 2)
    {
        Log::Warn("Usage: {} <slot|steamid64> [param]", args.Arg(0));
        return;
    }

    const int slot = ResolveSlot(args.Arg(1));
    if (!IsValidSlot(slot))
    {
        Log::Warn("'{}' is not a live slot or steamid64.", args.Arg(1));
        return;
    }

    // The filter rewrites live player commands, so it stays uninstalled until the first BeginPattern. A
    // disabled simulator then costs nothing on the per-tick movement path.
    if (!_filter)
        _filter = _rt.Hooks.Movement.Rewrite +=
            [this](int filtered, VoltMod::PlayerInput& cmd) { OnFilter(filtered, cmd); };

    auto& state = _sim[slot];
    state = {};
    state.kind = kind;
    state.param = args.ArgC() > 2 ? std::strtof(args.Arg(2), nullptr) : defaultParam;
    state.expireAt = Time::MonotonicSeconds() + SimulationSeconds;
    if (kind == Kind::Names)
        state.baseName = std::string(_rt.Entities.Controller(slot).Name());
    Log::Info("Simulating slot {} (param {:.1f}) for {:.0f}s.", slot, state.param, SimulationSeconds);
}

bool CheatSimulator::AimAtNearestOpponent(int slot, VoltMod::PlayerInput& cmd)
{
    const VoltMod::Pawn self = _rt.Entities.PawnOf(slot);
    if (!self)
        return false;

    const Vector eye = self.EyePosition();
    const Vec3 from{eye.x, eye.y, eye.z};
    const int team = self.Team();
    // The correlator's copy of the free-for-all rule is kept current by RefreshTeamRules, so the
    // simulation opposes exactly who the detectors do - and pays no name lookup per usercmd.
    const ShotHistory& correlator = _detectors.History;

    Vec3 best;
    float bestDistance = 0.0f;
    bool found = false;
    for (const VoltMod::Player* player : _rt.Players.All())
    {
        const int other = player ? player->Slot() : -1;
        if (!IsValidSlot(other) || other == slot)
            continue;
        const VoltMod::Pawn pawn = _rt.Entities.PawnOf(other);
        if (!pawn || !pawn.IsAlive() || !correlator.AreOpponents(team, pawn.Team()))
            continue;

        const Vector origin = pawn.Origin();
        const Vec3 target{origin.x, origin.y, origin.z + LockHeight};
        const float distance = (target - from).Length();
        if (!found || distance < bestDistance)
        {
            bestDistance = distance;
            best = target;
            found = true;
        }
    }
    if (!found)
        return false;

    const AimAngles aim = Geometry::Bearing(from, best);
    cmd.ViewPitch = aim.Pitch;
    cmd.ViewYaw = aim.Yaw;
    return true;
}

void CheatSimulator::OnFilter(int slot, VoltMod::PlayerInput& cmd)
{
    if (!Enabled() || !cmd.Valid || !IsValidSlot(slot))
        return;

    auto& state = _sim[slot];
    if (state.kind == Kind::Off)
        return;
    if (Time::MonotonicSeconds() > state.expireAt)
    {
        if (state.kind == Kind::Names)
            _rt.Entities.Controller(slot).SetName(state.baseName);
        state.kind = Kind::Off;
        return;
    }
    if (!state.anchored)
    {
        state.baseYaw = cmd.ViewYaw;
        state.anchored = true;
    }

    switch (state.kind)
    {
    case Kind::Spin:
    {
        const float step = state.param / TickRate;
        state.spinYaw = std::fmod(state.spinYaw + step, 360.0f);
        cmd.ViewYaw = state.spinYaw;
        cmd.SubtickMoveCount = 1;
        cmd.SubtickMoves[0] = {};
        cmd.SubtickMoves[0].YawDelta = step;
        break;
    }
    case Kind::Jitter:
        // Exactly repeating yaws: the same three values in the same order, command after command.
        cmd.ViewYaw = state.baseYaw + state.param * static_cast<float>(state.step % JitterPeriod);
        break;
    case Kind::BadAngles:
        cmd.ViewPitch = state.param;
        cmd.ViewRoll = BadRoll;
        break;
    case Kind::Aimlock:
        AimAtNearestOpponent(slot, cmd);
        break;
    case Kind::Mismatch:
        // What AntiAim's base-vs-history rule reads. SilentAim measures real impact geometry, so
        // editing the decoded view never reaches it.
        cmd.InputHistorySampleCount = 1;
        cmd.InputHistoryTotalCount = 1;
        cmd.InputHistorySamples[0] = {
            .HasViewAngles = true, .ViewPitch = cmd.ViewPitch, .ViewYaw = cmd.ViewYaw + state.param};
        cmd.Attack1StartHistoryIndex = (cmd.ButtonsHeld & VoltMod::IN_ATTACK) != 0 ? 0 : -1;
        break;
    case Kind::NoMouse:
        // The view lands on the enemy while the counts say the mouse never moved.
        AimAtNearestOpponent(slot, cmd);
        cmd.MouseDx = 0;
        cmd.MouseDy = 0;
        break;
    case Kind::Names:
        if (state.step % RenameEveryCommands == 0)
            _rt.Entities.Controller(slot).SetName(
                std::format("{}~{}", state.baseName, state.step / RenameEveryCommands));
        break;
    case Kind::Off:
        break;
    }
    ++state.step;
}

}  // namespace Anticheat
