#include "AntiCheatBridge.hpp"

#include "../Core/Managers.hpp"
#include "../Core/Permissions.hpp"
#include "../Database/Entities/Ban.hpp"

#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/SteamId.hpp>
#include <charconv>
#include <cstring>
#include <string>
#include <tier1/convar.h>

namespace Log = CS2Kit::Utils::Log;
using CS2Kit::Core::Engine;

namespace AdminSystem::Core
{

namespace
{
int64_t ParseSteamId64(const char* arg)
{
    int64_t steamId = 0;
    auto [ptr, ec] = std::from_chars(arg, arg + std::strlen(arg), steamId);
    return (ec == std::errc{} && CS2Kit::Utils::SteamId::IsValid(steamId)) ? steamId : 0;
}
}  // namespace

void AntiCheatBridge::RegisterConsoleCommands()
{
    _cmdBan.emplace("as_ac_ban", "Anticheat auto-ban: as_ac_ban <steamid64> <durationSec> <reason...>",
                    [](const CCommand& args) {
                        if (args.ArgC() < 4)
                        {
                            Log::Warn("Usage: as_ac_ban <steamid64> <durationSec> <reason...>");
                            return;
                        }

                        int64_t steamId = ParseSteamId64(args.Arg(1));
                        if (steamId == 0)
                        {
                            Log::Warn("as_ac_ban: '{}' is not a SteamID64.", args.Arg(1));
                            return;
                        }

                        int64_t durationSec = 0;
                        const char* durationArg = args.Arg(2);
                        std::from_chars(durationArg, durationArg + std::strlen(durationArg), durationSec);

                        std::string reason;
                        for (int i = 3; i < args.ArgC(); ++i)
                        {
                            if (!reason.empty())
                                reason += ' ';
                            reason += args.Arg(i);
                        }

                        Database::Ban ban;
                        ban.TargetSteamId = steamId;
                        ban.AdminSteamId = 0;
                        ban.AdminName = "AntiCheat";
                        ban.Reason = reason;
                        ban.Duration = durationSec;
                        if (auto* target = Engine().Players.GetPlayerBySteamId(steamId))
                        {
                            ban.TargetName = target->GetName();
                            ban.TargetIp = target->GetIpAddress();
                        }

                        if (App().Punishments.IssueBan(ban))
                            Log::Info("as_ac_ban: banned {} ({}s): {}", steamId, durationSec, reason);
                        else
                            Log::Warn("as_ac_ban: failed to persist ban for {}.", steamId);
                    });

    _cmdAlert.emplace(
        "as_ac_alert", "Anticheat admin alert: as_ac_alert <steamid64> <detector> <score>", [](const CCommand& args) {
            if (args.ArgC() < 4)
            {
                Log::Warn("Usage: as_ac_alert <steamid64> <detector> <score>");
                return;
            }

            int64_t steamId = ParseSteamId64(args.Arg(1));
            if (steamId == 0)
            {
                Log::Warn("as_ac_alert: '{}' is not a SteamID64.", args.Arg(1));
                return;
            }

            auto* suspect = Engine().Players.GetPlayerBySteamId(steamId);
            std::string suspectName = suspect ? suspect->GetName() : std::to_string(steamId);

            for (auto* admin : Engine().Players.GetAllPlayers())
            {
                if (!App().Admins.HasPermission(admin->GetSteamID(), Permission::Ban))
                    continue;
                Engine().Messages.ReplyKey(admin->GetSlot(), "anticheat.alert",
                                           {{"name", suspectName}, {"detector", args.Arg(2)}, {"score", args.Arg(3)}});
            }
        });
}

}  // namespace AdminSystem::Core
