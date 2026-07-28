#pragma once

// Dev-only: synthesises cheat-shaped input by rewriting the decoded UserCmdView (the game runs the
// real command untouched, so nothing here changes what the player's bullets actually do). Gated by
// anticheat.debug.simulator, which decides whether the arming commands exist at all; the movement
// filter is installed only once a pattern is armed.
//
// Each pattern targets one module: Spin and Jitter feed AntiAim's motion rules, BadAngles its
// invalid pitch/roll rule, Aimlock the tracking episodes, and Mismatch its base-vs-input-history
// divergence rule. SilentAim is deliberately not simulated: it measures where the bullet actually
// landed, which only the real usercmd the engine runs can move.

#include <CS2Kit/Api.hpp>
#include <cstdint>
#include <optional>

namespace Anticheat
{

class CheatSimulator
{
public:
    void Initialize();

private:
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

    void OnFilter(int slot, CS2Kit::UserCmdView& cmd);
    void Arm(const CCommand& args, Kind kind, float defaultParam);
    /** Point the command at the nearest opponent's chest; false when there is nobody to lock onto. */
    static bool AimAtNearestOpponent(int slot, CS2Kit::UserCmdView& cmd);

    static int ResolveSlot(const char* arg);

    bool Enabled() const;

    CS2Kit::PerSlot<SimState> _sim;
    bool _filtering = false;  // movement filter installed (lazily, on the first Arm)
    std::optional<CS2Kit::ServerCommand> _cmdSpin;
    std::optional<CS2Kit::ServerCommand> _cmdJitter;
    std::optional<CS2Kit::ServerCommand> _cmdBadAngles;
    std::optional<CS2Kit::ServerCommand> _cmdAimlock;
    std::optional<CS2Kit::ServerCommand> _cmdMismatch;
    std::optional<CS2Kit::ServerCommand> _cmdOff;
};

}  // namespace Anticheat
