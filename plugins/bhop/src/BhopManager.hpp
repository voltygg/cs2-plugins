#pragma once

#include "Config.hpp"
#include "MovementConVars.hpp"

#include <CS2Kit/Api.hpp>
#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <unordered_set>
#include <vector>

namespace Bhop
{

/**
 * Bhop policy: server-wide mode, per-player session grants, and the hop-chain boost. The convar
 * mechanics both modes rely on live in MovementConVars; this class decides when to drive them.
 *
 * "enabled" mode sets the configured movement convars through the engine path, so they replicate
 * to every client and the whole feature is client-predicted (ping-free). "grants" mode leaves the
 * server untouched and enables bhop per player: the client gets the convar values via
 * ReplicateToClient (its prediction auto-jumps) while the server flips the same convars only
 * around that player's RunCommand via the kit MovementHook.
 */
class BhopManager
{
public:
    BhopManager(CS2Kit::Runtime& runtime, ConfigManager& config)
        : _rt(runtime), _config(config), _conVars(runtime.ConVars)
    {}

    /** Parse settings, apply the mode, register listeners and console commands. */
    void Initialize();

    /** Session grant/revoke for @p steamId; applies immediately when the player is online. */
    void Grant(int64_t steamId, bool enabled);
    bool IsGranted(int64_t steamId) const { return _granted.contains(steamId); }

    /** Re-read settings.jsonc and re-apply (bhop_reload): restores prior convar values first. */
    void ReloadSettings();

    void OnPlayerDisconnect(CS2Kit::Player* player);

private:
    enum class Mode : uint8_t
    {
        Enabled,
        Grants,
    };

    void RegisterConsoleCommands();  // ConsoleCommands.cpp
    void ApplySettings();

    void OnRunCommandPre(int slot);
    void OnRunCommandPost(int slot);
    void OnPlayerJump(int slot);
    void OnPlayerSpawn(int slot);
    void ForceAutoHop(int slot);

    bool IsActiveSlot(int slot) const;

    CS2Kit::Runtime& _rt;
    ConfigManager& _config;
    Mode _mode = Mode::Enabled;
    MovementConVars _conVars;

    // Held for the life of the manager; each unregisters when this object goes.
    /** Listener registrations, released together. Declared last: reverse member destruction
     *  stops the callbacks before the state they capture goes away. */
    std::vector<CS2Kit::Subscription> _subs;

    std::unordered_set<int64_t> _granted;
    // Per-slot mirror of _granted for the movement hot path, where a steamId lookup per tick
    // would be wasteful. Kept in lockstep with _granted by Grant/OnPlayerSpawn/OnPlayerDisconnect.
    std::array<bool, CS2Kit::MaxPlayers> _grantedSlots{};
    std::array<std::chrono::steady_clock::time_point, CS2Kit::MaxPlayers> _lastJump{};
};

}  // namespace Bhop
