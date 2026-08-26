#include "CheatSimulator.hpp"

#include "AntiCheatManager.hpp"
#include "App.hpp"
#include "Core/Geometry.hpp"
#include "Core/Samples.hpp"
#include "Correlation/ShotCorrelatorCore.hpp"

#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Slot.hpp>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <mathlib/vector.h>
#include <tier1/convar.h>

namespace Anticheat
{

using VoltMod::IsValidSlot;
namespace Log = VoltMod::Log;

static constexpr double SimulationSeconds = 10.0;
/** Three is the middle jitter period AntiAim looks for. */
static constexpr int JitterPeriod = 3;
/** Well past AntiAim's 50.01 degree roll ceiling. */
static constexpr float BadRoll = 60.0f;
/** Chest height, so the lock reads as a body point rather than a graze. */
static constexpr float LockHeight = 46.0f;

void CheatSimulator::Initialize()
{
    _sim.BindReset(_rt.Slots);

    if (!Enabled())
    {
        Log::Info("Cheat simulator idle; enable anticheat.debug.simulator and reload the plugin to arm it.");
        return;
    }

    _cmdSpin.emplace("anticheat_sim_spin", "Sim spinbot: anticheat_sim_spin <slot|steamid64> [degPerSec=720]",
                     [this](const CCommand& args) { Arm(args, Kind::Spin, 720.0f); });
    _cmdJitter.emplace("anticheat_sim_jitter", "Sim yaw jitter: anticheat_sim_jitter <slot|steamid64> [stepDeg=20]",
                       [this](const CCommand& args) { Arm(args, Kind::Jitter, 20.0f); });
    _cmdBadAngles.emplace("anticheat_sim_badangles",
                          "Sim impossible pitch and roll: anticheat_sim_badangles <slot|steamid64> [pitch=89.5]",
                          [this](const CCommand& args) { Arm(args, Kind::BadAngles, 89.5f); });
    _cmdAimlock.emplace("anticheat_sim_aimlock",
                        "Sim locking onto the nearest opponent: anticheat_sim_aimlock <slot|steamid64>",
                        [this](const CCommand& args) { Arm(args, Kind::Aimlock, 0.0f); });
    _cmdMismatch.emplace(
        "anticheat_sim_mismatch",
        "Sim input-history angles diverging from the view: anticheat_sim_mismatch <slot|steamid64> [deg=130]",
        [this](const CCommand& args) { Arm(args, Kind::Mismatch, 130.0f); });
    _cmdOff.emplace("anticheat_sim_off", "Stop simulating: anticheat_sim_off [slot|steamid64] (omit to clear all)",
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

    Log::Info("Cheat simulator armed and ready (anticheat_sim_*).");
}

bool CheatSimulator::Enabled() const
{
    return _config.Get().anticheat.debug.simulator;
}

int CheatSimulator::ResolveSlot(const char* arg)
{
    if (std::strlen(arg) > 10)  // too long to be a slot index; treat as a steamid64
    {
        const int64_t steamId = std::strtoll(arg, nullptr, 10);
        auto* player = _rt.Players.GetPlayerBySteamId(steamId);
        return player ? player->GetSlot() : -1;
    }
    return std::atoi(arg);
}

void CheatSimulator::Arm(const CCommand& args, Kind kind, float defaultParam)
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

    // The filter rewrites live player commands, so it stays uninstalled until the first Arm. A
    // disabled simulator then costs nothing on the per-tick movement path.
    if (!_filter)
        _filter = _rt.MovementHook.FilterCmd +=
            [this](int filtered, VoltMod::UserCmdView& cmd) { OnFilter(filtered, cmd); };

    auto& state = _sim[slot];
    state = {};
    state.kind = kind;
    state.param = args.ArgC() > 2 ? std::strtof(args.Arg(2), nullptr) : defaultParam;
    state.expireAt = Time::MonotonicSeconds() + SimulationSeconds;
    Log::Info("Simulating slot {} (param {:.1f}) for {:.0f}s.", slot, state.param, SimulationSeconds);
}

bool CheatSimulator::AimAtNearestOpponent(int slot, VoltMod::UserCmdView& cmd)
{
    const VoltMod::PlayerController self = _rt.Entities.Controller(slot);
    if (!self.IsValid() || !self.GetPawn())
        return false;

    const Vector eye = self.GetEyePosition();
    const Vec3 from{eye.x, eye.y, eye.z};
    const int team = self.GetTeam();
    auto teammatesAreEnemies = VoltMod::ConVar<bool>::Find(_rt.ConVars, "mp_teammates_are_enemies");
    const bool freeForAll = teammatesAreEnemies && teammatesAreEnemies->Get();

    Vec3 best;
    float bestDistance = 0.0f;
    bool found = false;
    for (const VoltMod::Player* player : _rt.Players.GetAllPlayers())
    {
        const int other = player ? player->GetSlot() : -1;
        if (!IsValidSlot(other) || other == slot)
            continue;
        const VoltMod::PlayerController controller = _rt.Entities.Controller(other);
        if (!controller.IsValid() || !controller.GetPawn() || !controller.IsAlive() ||
            !ShotCorrelatorCore::AreOpponents(team, controller.GetTeam(), freeForAll))
            continue;

        const Vector origin = controller.GetAbsOrigin();
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

void CheatSimulator::OnFilter(int slot, VoltMod::UserCmdView& cmd)
{
    if (!Enabled() || !cmd.Valid || !IsValidSlot(slot))
        return;

    auto& state = _sim[slot];
    if (state.kind == Kind::Off)
        return;
    if (Time::MonotonicSeconds() > state.expireAt)
    {
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
        // Rewrite the claimed input-history angles away from the visible view, which is what
        // AntiAim's base-vs-history rule reads. SilentAim judges real impact geometry, so an edit
        // to the decoded view never reaches it.
        cmd.InputHistorySampleCount = 1;
        cmd.InputHistoryTotalCount = 1;
        cmd.InputHistorySamples[0] = {
            .HasViewAngles = true, .ViewPitch = cmd.ViewPitch, .ViewYaw = cmd.ViewYaw + state.param};
        cmd.Attack1StartHistoryIndex = (cmd.ButtonsHeld & VoltMod::IN_ATTACK) != 0 ? 0 : -1;
        break;
    case Kind::Off:
        break;
    }
    ++state.step;
}

}  // namespace Anticheat
