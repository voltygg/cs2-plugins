#pragma once

#include "Detectors.hpp"
#include "Config.hpp"

#include <VoltMod/Api.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace Anticheat
{

class CheatSimulator
{
public:
    CheatSimulator(Detectors& detectors, VoltMod::Runtime& runtime, ConfigManager& config)
        : _detectors(detectors), _rt(runtime), _config(config)
    {}

    void Initialize();

private:
    Detectors& _detectors;
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
        NoMouse,
        Names,
    };

    struct SimState
    {
        Kind kind = Kind::Off;
        float param = 0.0f;    // pattern-specific magnitude (deg/sec, degrees, ...)
        float spinYaw = 0.0f;  // accumulated spin angle
        float baseYaw = 0.0f;  // jitter anchor, captured on the first rewritten command
        int step = 0;          // commands rewritten so far, for the jitter cycle
        std::string baseName;  // what Names restores when it expires
        bool anchored = false;
        double expireAt = 0.0;
    };

    void OnFilter(int slot, VoltMod::PlayerInput& cmd);
    void Start(const CCommand& args, Kind kind, float defaultParam);
    /** Point the command at the nearest opponent's chest; false with nobody to lock onto. */
    bool AimAtNearestOpponent(int slot, VoltMod::PlayerInput& cmd);

    int ResolveSlot(std::string_view arg);

    bool Enabled() const;

    VoltMod::PerSlot<SimState> _sim;
    // Movement filter, installed lazily on the first Start; empty while the simulator is idle.
    VoltMod::Subscription _filter;
    std::optional<VoltMod::ServerCommand> _cmdSpin;
    std::optional<VoltMod::ServerCommand> _cmdJitter;
    std::optional<VoltMod::ServerCommand> _cmdBadAngles;
    std::optional<VoltMod::ServerCommand> _cmdAimlock;
    std::optional<VoltMod::ServerCommand> _cmdMismatch;
    std::optional<VoltMod::ServerCommand> _cmdNoMouse;
    std::optional<VoltMod::ServerCommand> _cmdNames;
    std::optional<VoltMod::ServerCommand> _cmdOff;
};

}  // namespace Anticheat
