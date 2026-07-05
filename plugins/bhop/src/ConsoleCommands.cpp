#include "BhopManager.hpp"

#include <CS2Kit/Utils/Log.hpp>
#include <charconv>
#include <cstring>
#include <tier1/convar.h>
#include <utility>

namespace Log = CS2Kit::Utils::Log;

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

    // Live A/B switch for how grants mode produces the server-side hop (see HopStrategy).
    _cmdStrategy.emplace("bhop_strategy", "Grants hop strategy: bhop_strategy <off|velocity|press|both>",
                         [this](const CCommand& args) {
                             static constexpr std::pair<const char*, HopStrategy> Names[] = {
                                 {"off", HopStrategy::Off},
                                 {"velocity", HopStrategy::Velocity},
                                 {"press", HopStrategy::Press},
                                 {"both", HopStrategy::Both},
                             };

                             if (args.ArgC() >= 2)
                             {
                                 for (const auto& [name, value] : Names)
                                 {
                                     if (std::strcmp(args.Arg(1), name) == 0)
                                     {
                                         _strategy = value;
                                         Log::Info("bhop_strategy: {}.", name);
                                         return;
                                     }
                                 }
                                 Log::Warn("Usage: bhop_strategy <off|velocity|press|both>");
                                 return;
                             }

                             for (const auto& [name, value] : Names)
                                 if (value == _strategy)
                                     Log::Info("bhop_strategy is {}.", name);
                         });
}

}  // namespace Bhop
