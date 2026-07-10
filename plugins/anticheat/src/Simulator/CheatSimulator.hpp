#pragma once

// Dev-only: synthesises spinbot/aimlock/silent-aim by rewriting the decoded UserCmdView (the
// game runs the real command untouched). Gated at runtime by anticheat.debug.simulator.enabled,
// which every arming command checks; the movement filter is installed only once one is armed.

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Sdk/UserCmd.hpp>
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
        Aimlock,
        Silent
    };

    struct SimState
    {
        Kind kind = Kind::Off;
        float param = 0.0f;    // spin deg/sec, aimlock snap deg, or silent divergence deg
        float spinYaw = 0.0f;  // accumulated spin angle
        float lockYaw = 0.0f;
        float lockPitch = 0.0f;
        bool armed = false;  // aimlock: compute the lock target on the next tick
        int holdTicks = 0;   // aimlock snap-then-lock countdown
        double expireAt = 0.0;
    };

    void OnFilter(int slot, CS2Kit::UserCmdView& cmd);
    void Arm(const CCommand& args, Kind kind, float defaultParam);

    static int ResolveSlot(const char* arg);

    bool Enabled() const;

    CS2Kit::PerSlot<SimState> _sim;
    bool _filtering = false;  // movement filter installed (lazily, on the first Arm)
    std::optional<CS2Kit::ServerCommand> _cmdSpin;
    std::optional<CS2Kit::ServerCommand> _cmdAimlock;
    std::optional<CS2Kit::ServerCommand> _cmdSilent;
    std::optional<CS2Kit::ServerCommand> _cmdOff;
};

}  // namespace Anticheat
