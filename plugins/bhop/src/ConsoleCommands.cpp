#include "BhopManager.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <charconv>
#include <cstring>
#include <tier1/convar.h>

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

    // Diagnostic only: holds the raw server-side overrides applied for everyone (no per-player
    // scoping, nothing replicated), to verify the engine's movement code honors raw writes.
    // NOT a "bhop for everyone" switch - non-granted clients get no replication, so they would
    // rubber-band; use mode "enabled" for that.
    _cmdFlipHold.emplace("bhop_flip_hold", "Diagnostic: hold (1) or release (0) the raw bhop convar overrides.",
                         [this](const CCommand& args) {
                             if (args.ArgC() < 2)
                             {
                                 Log::Warn("Usage: bhop_flip_hold <0|1>");
                                 return;
                             }

                             bool hold = std::strcmp(args.Arg(1), "0") != 0;
                             _conVars.HoldRaw(hold);
                             Log::Info("bhop_flip_hold: raw overrides {}.", hold ? "held" : "released");
                         });

    _cmdDebug.emplace("bhop_debug", "Print movement-hook counters and grant state for live diagnosis.",
                      [this](const CCommand&) {
                          const auto& stats = CS2Kit::Core::Engine().MovementHook.GetStats();
                          int granted = 0;
                          for (bool slotGranted : _grantedSlots)
                              granted += slotGranted ? 1 : 0;
                          Log::Info("bhop_debug: usercmds={} movement={} scopes={} unresolved={} "
                                    "grantedSlots={} mode={}.",
                                    stats.UsercmdsCalls, stats.MovementCalls, stats.ScopesEntered,
                                    stats.UnresolvedSlots, granted, _mode == Mode::Grants ? "grants" : "enabled");
                      });
}

}  // namespace Bhop
