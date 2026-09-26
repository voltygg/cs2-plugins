#include "BhopManager.hpp"

#include <VoltMod/Api.hpp>
#include <format>

using VoltMod::Caller;
using VoltMod::Reply;

namespace Args = VoltMod::Args;

namespace Bhop
{

// Server console commands are the cross-plugin surface: admin-system (a separate module)
// drives grants through `bhop_player` via ExecuteServerCommand, and operators use both
// commands from the server console or cfg files.
void BhopManager::RegisterConsoleCommands()
{
    auto& commands = _rt.Commands;

    commands.Add("bhop_player")
        .Describe("Grant/revoke session bhop for a player.")
        .ServerOnly()
        .Run([this](Caller, Args::SteamId steamId, Args::Int enabled) {
            const bool granted = enabled.Value != 0;
            Grant(steamId.Value, granted);
            return Reply{std::format("bhop_player: {} for {}.", granted ? "granted" : "revoked", steamId.Value)};
        });

    commands.Add("bhop_reload")
        .Describe("Re-read settings.jsonc and re-apply the bhop configuration.")
        .ServerOnly()
        .Run([this](Caller) { ReloadSettings(); });
}

}  // namespace Bhop
