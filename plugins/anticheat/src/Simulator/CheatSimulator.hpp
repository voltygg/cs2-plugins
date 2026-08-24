#pragma once

// Dev-only: synthesises cheat-shaped input by rewriting the decoded UserCmdView. The game still
// runs the real command, so nothing here changes where the player's bullets go.
// anticheat.debug.simulator decides whether the arming commands exist at all.
//
// Each pattern targets one module: Spin and Jitter feed AntiAim's motion rules, BadAngles its
// invalid pitch/roll rule, Aimlock the tracking episodes, and Mismatch its base-vs-input-history
// divergence rule. SilentAim is deliberately absent - only the real usercmd can move where the
// bullet actually landed.

#include "../Config.hpp"

#include <VoltMod/Api.hpp>
#include <cstdint>
#include <optional>

namespace Anticheat
{

class AntiCheatManager;

class CheatSimulator
{
public:
    CheatSimulator(AntiCheatManager& manager, VoltMod::Runtime& runtime, ConfigManager& config)
        : _manager(manager), _rt(runtime), _config(config)
    {}

    void Initialize();

private:
    AntiCheatManager& _manager;
    VoltMod::Runtime& _rt;
    ConfigManager& _config;

    enum class Kind
    {
        Off,
        Spin,
        Jitter,
        BadAngles,
        Aimlock,
        Mismatch,
    };

    struct SimState
    {
        Kind kind = Kind::Off;
        float param = 0.0f;    // pattern-specific magnitude (deg/sec, degrees, ...)
        float spinYaw = 0.0f;  // accumulated spin angle
        float baseYaw = 0.0f;  // jitter anchor, captured on the first rewritten command
        int step = 0;          // commands rewritten so far, for the jitter cycle
        bool anchored = false;
        double expireAt = 0.0;
    };

    void OnFilter(int slot, VoltMod::UserCmdView& cmd);
    void Arm(const CCommand& args, Kind kind, float defaultParam);
    /** Point the command at the nearest opponent's chest; false with nobody to lock onto. */
    bool AimAtNearestOpponent(int slot, VoltMod::UserCmdView& cmd);

    int ResolveSlot(const char* arg);

    bool Enabled() const;

    VoltMod::PerSlot<SimState> _sim;
    // Movement filter, installed lazily on the first Arm; empty while the simulator is idle.
    VoltMod::Subscription _filter;
    std::optional<VoltMod::ServerCommand> _cmdSpin;
    std::optional<VoltMod::ServerCommand> _cmdJitter;
    std::optional<VoltMod::ServerCommand> _cmdBadAngles;
    std::optional<VoltMod::ServerCommand> _cmdAimlock;
    std::optional<VoltMod::ServerCommand> _cmdMismatch;
    std::optional<VoltMod::ServerCommand> _cmdOff;
};

}  // namespace Anticheat
