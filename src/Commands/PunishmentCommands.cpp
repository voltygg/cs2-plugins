#include "PunishmentCommands.hpp"
#include "../Core/Managers.hpp"

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
using AdminSystem::Database::TextMute;
using AdminSystem::Database::VoiceMute;
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
    Sys().Chat.BroadcastPunishment("kicked", admin->GetName(), target->GetName(), reason, 0);
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

    std::string reason = JoinReason(args, 2, Sys().Config.GetPunishments().defaultBanReason);

    Ban ban;
    FillPunishment(ban, target, admin, reason, durationSec);
    ban.TargetIp = target->GetIpAddress();

    if (!Sys().Punishments.IssueBan(ban))
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

    if (!Sys().Punishments.RemoveBanBySteamId(steamId, admin->GetSteamID(), reason))
    {
        return {false, std::format("No active ban for SteamID {}.", steamId)};
    }
    return {true, std::format("Unbanned {}.", steamId)};
}

CommandResult HandleVoiceMute(Player* admin, const std::vector<std::string>& args)
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

    VoiceMute mute;
    FillPunishment(mute, target, admin, JoinReason(args, 2, "Voice-muted by admin"), durationSec);

    if (!Sys().Punishments.IssueVoiceMute(mute))
    {
        return {false, "Failed to issue voice mute (database error)."};
    }
    return {true, std::format("Voice-muted {}.", target->GetName())};
}

CommandResult HandleVoiceUnmute(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    if (!Sys().Punishments.RemoveVoiceMuteBySteamId(target->GetSteamID(), admin->GetSteamID(),
                                                                "Voice-unmuted by admin"))
    {
        return {false, std::format("{} is not voice-muted.", target->GetName())};
    }
    return {true, std::format("Voice-unmuted {}.", target->GetName())};
}

CommandResult HandleTextMute(Player* admin, const std::vector<std::string>& args)
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

    TextMute mute;
    FillPunishment(mute, target, admin, JoinReason(args, 2, "Text-muted by admin"), durationSec);

    if (!Sys().Punishments.IssueTextMute(mute))
    {
        return {false, "Failed to issue text mute (database error)."};
    }
    return {true, std::format("Text-muted {}.", target->GetName())};
}

CommandResult HandleTextUnmute(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    if (!Sys().Punishments.RemoveTextMuteBySteamId(target->GetSteamID(), admin->GetSteamID(),
                                                               "Text-unmuted by admin"))
    {
        return {false, std::format("{} is not text-muted.", target->GetName())};
    }
    return {true, std::format("Text-unmuted {}.", target->GetName())};
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

    if (!Sys().Punishments.IssueWarning(warn))
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

    mgr.Register(CommandBuilder("voice_mute")
                     .WithAliases({"vmute"})
                     .WithDescription("Voice-mute a player for the given number of minutes (0 = permanent).")
                     .WithUsage("!voice_mute <target> <minutes> [reason]")
                     .RequirePermission("o")
                     .WithArgs(2)
                     .OnExecute(HandleVoiceMute)
                     .Build());

    mgr.Register(CommandBuilder("voice_unmute")
                     .WithAliases({"vunmute"})
                     .WithDescription("Lift an active voice mute on the target.")
                     .WithUsage("!voice_unmute <target>")
                     .RequirePermission("o")
                     .WithArgs(1, 1)
                     .OnExecute(HandleVoiceUnmute)
                     .Build());

    mgr.Register(
        CommandBuilder("text_mute")
            .WithAliases({"tmute"})
            .WithDescription("Text-mute (chat-block) a player for the given number of minutes (0 = permanent).")
            .WithUsage("!text_mute <target> <minutes> [reason]")
            .RequirePermission("p")
            .WithArgs(2)
            .OnExecute(HandleTextMute)
            .Build());

    mgr.Register(CommandBuilder("text_unmute")
                     .WithAliases({"tunmute"})
                     .WithDescription("Lift an active text mute on the target.")
                     .WithUsage("!text_unmute <target>")
                     .RequirePermission("p")
                     .WithArgs(1, 1)
                     .OnExecute(HandleTextUnmute)
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
