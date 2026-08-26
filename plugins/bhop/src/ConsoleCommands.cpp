#include "BhopManager.hpp"

#include <VoltMod/Api.hpp>
#include <format>

using VoltMod::CommandContext;
using VoltMod::CommandResult;
using VoltMod::Int;
using VoltMod::SteamId64;
using VoltMod::Surface;

namespace Bhop
{

// Server console commands are the cross-plugin surface: admin-system (a separate module)
// drives grants through `bhop_player` via ExecuteServerCommand, and operators use both
// commands from the server console or cfg files.
void BhopManager::RegisterConsoleCommands()
{
    auto& commands = _rt.Commands;

    commands.Register({
        .Name = "bhop_player",
        .Description = "Grant/revoke session bhop for a player.",
        .Args = {SteamId64(), Int()},
        .Surfaces = Surface::Console,
        .Handler =
            [this](CommandContext& c) {
                const bool enabled = c.Int().value_or(0) != 0;
                Grant(c.SteamId, enabled);
                return CommandResult{
                    std::format("bhop_player: {} for {}.", enabled ? "granted" : "revoked", c.SteamId)};
            },
    });

    commands.Register({
        .Name = "bhop_reload",
        .Description = "Re-read settings.jsonc and re-apply the bhop configuration.",
        .Surfaces = Surface::Console,
        .Handler =
            [this](CommandContext&) {
                ReloadSettings();
                return CommandResult::Silent();
            },
    });
}

}  // namespace Bhop
