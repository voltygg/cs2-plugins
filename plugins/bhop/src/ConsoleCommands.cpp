#include "BhopManager.hpp"

#include <CS2Kit/Core/Log.hpp>
#include <charconv>
#include <cstring>
#include <tier1/convar.h>

namespace Log = CS2Kit::Log;

namespace Bhop
{

// Server console commands are the cross-plugin surface: admin-system (a separate module)
// drives grants through `bhop_player` via ExecuteServerCommand, and operators use both
// commands from the server console or cfg files.
void BhopManager::RegisterConsoleCommands()
{
    _cmdPlayer.emplace("bhop_player", "Grant/revoke session bhop for a player: bhop_player <steamid64> <0|1>",
                       [this](const CCommand& args) {
                           if (args.ArgC() < 3)
                           {
                               Log::Warn("Usage: bhop_player <steamid64> <0|1>");
                               return;
                           }

                           int64_t steamId = 0;
                           const char* arg = args.Arg(1);
                           auto [ptr, ec] = std::from_chars(arg, arg + std::strlen(arg), steamId);
                           if (ec != std::errc{} || steamId <= 0)
                           {
                               Log::Warn("bhop_player: '{}' is not a SteamID64.", arg);
                               return;
                           }

                           bool enabled = std::strcmp(args.Arg(2), "0") != 0;
                           Grant(steamId, enabled);
                           Log::Info("bhop_player: {} for {}.", enabled ? "granted" : "revoked", steamId);
                       });

    _cmdReload.emplace("bhop_reload", "Re-read settings.jsonc and re-apply the bhop configuration.",
                       [this](const CCommand&) { ReloadSettings(); });
}

}  // namespace Bhop
