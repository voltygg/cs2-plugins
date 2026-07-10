#pragma once

#include "PlayerMonitor.hpp"
#include "ResponseManager.hpp"
#include "Simulator/CheatSimulator.hpp"

#include <CS2Kit/Api.hpp>
#include <optional>

namespace Anticheat
{

/**
 * Wires the kit feeds (usercmd stream, game events) into the detectors and
 * routes their findings to the ResponseManager. Owns the per-slot detection
 * state and the anticheat_reload / anticheat_status console commands.
 */
class AntiCheatManager
{
public:
    explicit AntiCheatManager(ResponseManager& response) : _response(response) {}

    void Initialize();

private:
    void OnCmd(int slot, const CS2Kit::UserCmdView& cmd);
    void OnWeaponFire(const CS2Kit::Events::WeaponFire& e);
    void OnPlayerHurt(const CS2Kit::Events::PlayerHurt& e);
    void OnPlayerDeath(const CS2Kit::Events::PlayerDeath& e);
    void OnPlayerBlind(const CS2Kit::Events::PlayerBlind& e);

    /**
     * Angular error between the attacker's fire-tick aim and the direction to
     * the victim (best of head/chest). Large sentinel when either side is gone.
     */
    float AimErrorDeg(int attackerSlot, int victimSlot, const PlayerState& s) const;

    ResponseManager& _response;
    CS2Kit::PerSlot<PlayerState> _players;
    CS2Kit::PerSlot<int> _dumpTicks;  // remaining ticks to dump raw usercmds (anticheat_dumpcmd)
    std::optional<CS2Kit::ServerCommand> _cmdReload;
    std::optional<CS2Kit::ServerCommand> _cmdStatus;
    std::optional<CS2Kit::ServerCommand> _cmdDump;
    CheatSimulator _simulator;
};

}  // namespace Anticheat
