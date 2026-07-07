#pragma once

#include <CS2Kit/Api.hpp>
#include <optional>

namespace AdminSystem::Core
{

/**
 * Server-console bridge for the anticheat plugin (a separate module - console
 * commands are the standard cross-plugin surface):
 *
 *   as_ac_ban <steamid64> <durationSec> <reason...>  - console-originated ban
 *   as_ac_alert <steamid64> <detector> <score>       - notify Ban-flag admins
 *
 * Bans go through PunishmentManager::IssueBan directly with AdminSteamId=0 /
 * AdminName="AntiCheat": there is no admin Player to attribute, and automated
 * bans must not count against any admin's abuse-rate stats.
 */
class AntiCheatBridge
{
public:
    void RegisterConsoleCommands();

private:
    std::optional<CS2Kit::ServerCommand> _cmdBan;
    std::optional<CS2Kit::ServerCommand> _cmdAlert;
};

}  // namespace AdminSystem::Core
