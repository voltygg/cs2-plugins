#include "PunishmentCommands.hpp"

#include "../Core/ChatService.hpp"
#include "../Core/Config.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "CommandHelpers.hpp"

#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>

#include <format>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace CS2Kit::Players;
using namespace CS2Kit::Sdk;
using namespace CS2Kit::Utils;
using namespace AdminSystem::Commands::Helpers;
using AdminSystem::Core::ChatService;
using AdminSystem::Core::ConfigManager;
using AdminSystem::Database::Ban;
using AdminSystem::Database::Gag;
using AdminSystem::Database::Mute;
using AdminSystem::Database::Warning;
using AdminSystem::Punishments::PunishmentManager;

namespace
{

CommandResult HandleKick(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    std::string reason = JoinReason(args, 1, "Kicked by admin");
    PlayerController(target->GetSlot()).Kick(reason.c_str());
    ChatService::Instance().BroadcastPunishment("kicked", admin->GetName(), target->GetName(), reason, 0);
    return {true, std::format("Kicked {}.", target->GetName())};
}

CommandResult HandleBan(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    int64_t durationSec = 0;
    if (!ParseDurationMinutes(args[1], durationSec))
    {
        return {false, "Duration must be a non-negative number of minutes (0 = permanent)."};
    }

    std::string reason = JoinReason(args, 2, ConfigManager::Instance().GetPluginConfig().DefaultBanReason);

    Ban ban;
    FillPunishment(ban, target, admin, reason, durationSec);
    ban.TargetIp = target->GetIpAddress();

    if (!PunishmentManager::Instance().IssueBan(ban))
    {
        return {false, "Failed to issue ban (database error)."};
    }
    return {true, std::format("Banned {}.", target->GetName())};
}

CommandResult HandleUnban(Player* admin, const std::vector<std::string>& args)
{
    if (!StringUtils::IsNumeric(args[0]))
    {
        return {false, "Usage: !unban <steamId> [reason]"};
    }
    int64_t steamId = std::stoll(args[0]);
    std::string reason = JoinReason(args, 1, "Unbanned by admin");

    if (!PunishmentManager::Instance().RemoveBanBySteamId(steamId, admin->GetSteamID(), reason))
    {
        return {false, std::format("No active ban for SteamID {}.", steamId)};
    }
    return {true, std::format("Unbanned {}.", steamId)};
}

CommandResult HandleMute(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    int64_t durationSec = 0;
    if (!ParseDurationMinutes(args[1], durationSec))
    {
        return {false, "Duration must be a non-negative number of minutes (0 = permanent)."};
    }

    Mute mute;
    FillPunishment(mute, target, admin, JoinReason(args, 2, "Muted by admin"), durationSec);

    if (!PunishmentManager::Instance().IssueMute(mute))
    {
        return {false, "Failed to issue mute (database error)."};
    }
    return {true, std::format("Muted {}.", target->GetName())};
}

CommandResult HandleUnmute(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    if (!PunishmentManager::Instance().RemoveMuteBySteamId(target->GetSteamID(), admin->GetSteamID(),
                                                           "Unmuted by admin"))
    {
        return {false, std::format("{} is not muted.", target->GetName())};
    }
    return {true, std::format("Unmuted {}.", target->GetName())};
}

CommandResult HandleGag(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    int64_t durationSec = 0;
    if (!ParseDurationMinutes(args[1], durationSec))
    {
        return {false, "Duration must be a non-negative number of minutes (0 = permanent)."};
    }

    Gag gag;
    FillPunishment(gag, target, admin, JoinReason(args, 2, "Gagged by admin"), durationSec);

    if (!PunishmentManager::Instance().IssueGag(gag))
    {
        return {false, "Failed to issue gag (database error)."};
    }
    return {true, std::format("Gagged {}.", target->GetName())};
}

CommandResult HandleUngag(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    if (!PunishmentManager::Instance().RemoveGagBySteamId(target->GetSteamID(), admin->GetSteamID(),
                                                          "Ungagged by admin"))
    {
        return {false, std::format("{} is not gagged.", target->GetName())};
    }
    return {true, std::format("Ungagged {}.", target->GetName())};
}

CommandResult HandleWarn(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    Warning warn;
    FillPunishment(warn, target, admin, JoinReason(args, 1, "Warned by admin"), 0);

    if (!PunishmentManager::Instance().IssueWarning(warn))
    {
        return {false, "Failed to issue warning (database error)."};
    }
    return {true, std::format("Warned {}.", target->GetName())};
}

}  // namespace

void RegisterPunishmentCommands(CommandManager& mgr)
{
    mgr.Register(CommandBuilder("kick")
                     .WithDescription("Kick a player.")
                     .WithUsage("!kick <target> [reason]")
                     .RequirePermission("c")
                     .WithArgs(1)
                     .OnExecute(HandleKick)
                     .Build());

    mgr.Register(CommandBuilder("ban")
                     .WithDescription("Ban a player for the given number of minutes (0 = permanent).")
                     .WithUsage("!ban <target> <minutes> [reason]")
                     .RequirePermission("d")
                     .WithArgs(2)
                     .OnExecute(HandleBan)
                     .Build());

    mgr.Register(CommandBuilder("unban")
                     .WithDescription("Lift an active ban for the given SteamID.")
                     .WithUsage("!unban <steamId> [reason]")
                     .RequirePermission("e")
                     .WithArgs(1)
                     .OnExecute(HandleUnban)
                     .Build());

    mgr.Register(CommandBuilder("mute")
                     .WithDescription("Voice-mute a player for the given number of minutes (0 = permanent).")
                     .WithUsage("!mute <target> <minutes> [reason]")
                     .RequirePermission("o")
                     .WithArgs(2)
                     .OnExecute(HandleMute)
                     .Build());

    mgr.Register(CommandBuilder("unmute")
                     .WithDescription("Lift an active voice mute on the target.")
                     .WithUsage("!unmute <target>")
                     .RequirePermission("o")
                     .WithArgs(1, 1)
                     .OnExecute(HandleUnmute)
                     .Build());

    mgr.Register(CommandBuilder("gag")
                     .WithDescription("Chat-gag a player for the given number of minutes (0 = permanent).")
                     .WithUsage("!gag <target> <minutes> [reason]")
                     .RequirePermission("p")
                     .WithArgs(2)
                     .OnExecute(HandleGag)
                     .Build());

    mgr.Register(CommandBuilder("ungag")
                     .WithDescription("Lift an active chat gag on the target.")
                     .WithUsage("!ungag <target>")
                     .RequirePermission("p")
                     .WithArgs(1, 1)
                     .OnExecute(HandleUngag)
                     .Build());

    mgr.Register(CommandBuilder("warn")
                     .WithDescription("Issue a warning. Auto-escalates to a ban once the threshold is reached.")
                     .WithUsage("!warn <target> [reason]")
                     .RequirePermission("q")
                     .WithArgs(1)
                     .OnExecute(HandleWarn)
                     .Build());
}

}  // namespace AdminSystem::Commands
