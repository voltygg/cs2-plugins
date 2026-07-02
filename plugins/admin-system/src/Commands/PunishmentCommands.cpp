#include "PunishmentCommands.hpp"

#include "../Core/Config.hpp"
#include "../Core/Managers.hpp"
#include "../Core/Permissions.hpp"
#include "../Punishments/IssuePunishment.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "CommandHelpers.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;
using namespace AdminSystem::Commands::Helpers;
using namespace AdminSystem::Punishments;
using CS2Kit::Core::Engine;

namespace
{

/**
 * Shared body of the kick/ban/mute/warn handlers: resolve the target, issue via the common
 * entry point, and reply in the caller's language. Fallback reasons use the server language -
 * they land in the DB and in all-player broadcasts.
 */
CommandResult HandlePunish(Player* admin, const std::vector<std::string>& args, PunishType type,
                           std::size_t reasonStart, const std::string& fallbackReason, int64_t durationSec,
                           const char* successKey, const char* failedKey)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    // Captured before issuing: bans and kicks can drop the target immediately.
    std::string targetName = target->GetName();
    std::string reason = JoinReason(args, reasonStart, fallbackReason);

    if (!IssuePunishment(*admin, *target, type, reason, durationSec))
    {
        return {false, Engine().Translations.Get(failedKey, admin->GetSlot())};
    }
    return {true, Engine().Translations.Get(successKey, admin->GetSlot(), {{"name", targetName}})};
}

/** Like HandlePunish for the timed types: the duration is parsed from args[1]. */
CommandResult HandleTimedPunish(Player* admin, const std::vector<std::string>& args, PunishType type,
                                const std::string& fallbackReason, const char* successKey, const char* failedKey)
{
    int64_t durationSec = 0;
    if (!ParseCommandDuration(args[1], durationSec))
    {
        return {false, Engine().Translations.Get("cmd.badDuration", admin->GetSlot())};
    }
    return HandlePunish(admin, args, type, 2, fallbackReason, durationSec, successKey, failedKey);
}

CommandResult HandleKick(Player* admin, const std::vector<std::string>& args)
{
    // Kick has no DB write, so IssuePunishment cannot fail and the failure key is never read.
    return HandlePunish(admin, args, PunishType::Kick, 1, Engine().Translations.Get("reason.kickedByAdmin"), 0,
                        "cmd.kickSuccess", "cmd.kickSuccess");
}

CommandResult HandleBan(Player* admin, const std::vector<std::string>& args)
{
    return HandleTimedPunish(admin, args, PunishType::Ban, App().Config.GetPunishments().defaultBanReason,
                             "cmd.banSuccess", "cmd.banFailed");
}

CommandResult HandleUnban(Player* admin, const std::vector<std::string>& args)
{
    if (!StringUtils::IsNumeric(args[0]))
    {
        return {false, Engine().Translations.Get("cmd.unbanUsage", admin->GetSlot())};
    }
    int64_t steamId = std::stoll(args[0]);
    std::string reason = JoinReason(args, 1, Engine().Translations.Get("reason.unbannedByAdmin"));

    bool removed = App().Punishments.RemoveBanBySteamId(steamId, admin->GetSteamID(), reason);
    return {removed, Engine().Translations.Get(removed ? "cmd.unbanSuccess" : "cmd.unbanNoBan", admin->GetSlot(),
                                               {{"id", std::to_string(steamId)}})};
}

CommandResult HandleVoiceMute(Player* admin, const std::vector<std::string>& args)
{
    return HandleTimedPunish(admin, args, PunishType::VoiceMute, Engine().Translations.Get("reason.voiceMutedByAdmin"),
                             "cmd.voiceMuteSuccess", "cmd.voiceMuteFailed");
}

CommandResult HandleVoiceUnmute(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    bool removed = App().Punishments.RemoveVoiceMuteBySteamId(target->GetSteamID(), admin->GetSteamID(),
                                                              Engine().Translations.Get("reason.voiceUnmutedByAdmin"));
    return {removed, Engine().Translations.Get(removed ? "cmd.voiceUnmuteSuccess" : "cmd.voiceUnmuteNotMuted",
                                               admin->GetSlot(), {{"name", target->GetName()}})};
}

CommandResult HandleTextMute(Player* admin, const std::vector<std::string>& args)
{
    return HandleTimedPunish(admin, args, PunishType::TextMute, Engine().Translations.Get("reason.textMutedByAdmin"),
                             "cmd.textMuteSuccess", "cmd.textMuteFailed");
}

CommandResult HandleTextUnmute(Player* admin, const std::vector<std::string>& args)
{
    std::string err;
    Player* target = ResolveSingle(args[0], admin, err);
    if (!target)
    {
        return {false, err};
    }

    bool removed = App().Punishments.RemoveTextMuteBySteamId(target->GetSteamID(), admin->GetSteamID(),
                                                             Engine().Translations.Get("reason.textUnmutedByAdmin"));
    return {removed, Engine().Translations.Get(removed ? "cmd.textUnmuteSuccess" : "cmd.textUnmuteNotMuted",
                                               admin->GetSlot(), {{"name", target->GetName()}})};
}

CommandResult HandleWarn(Player* admin, const std::vector<std::string>& args)
{
    return HandlePunish(admin, args, PunishType::Warn, 1, Engine().Translations.Get("reason.warnedByAdmin"), 0,
                        "cmd.warnSuccess", "cmd.warnFailed");
}

}  // namespace

void RegisterPunishmentCommands(CommandManager& mgr)
{
    mgr.Register(CommandBuilder("kick")
                     .WithDescription("Kick a player.")
                     .WithUsage("!kick <target> [reason]")
                     .RequirePermission(Flag(Permission::Kick))
                     .WithArgs(1)
                     .OnExecute(HandleKick)
                     .Build());

    mgr.Register(
        CommandBuilder("ban")
            .WithDescription("Ban a player. Duration: minutes (e.g. 30) or 30s/5m/2h/7d; 0/'perm' = permanent.")
            .WithUsage("!ban <target> <duration> [reason]")
            .RequirePermission(Flag(Permission::Ban))
            .WithArgs(2)
            .OnExecute(HandleBan)
            .Build());

    mgr.Register(CommandBuilder("unban")
                     .WithDescription("Lift an active ban for the given SteamID.")
                     .WithUsage("!unban <steamId> [reason]")
                     .RequirePermission(Flag(Permission::Unban))
                     .WithArgs(1)
                     .OnExecute(HandleUnban)
                     .Build());

    mgr.Register(CommandBuilder("voice_mute")
                     .WithAliases({"vmute", "mute"})
                     .WithDescription("Voice-mute a player. Duration: minutes or 30s/5m/2h/7d; 0/'perm' = permanent.")
                     .WithUsage("!voice_mute <target> <duration> [reason]")
                     .RequirePermission(Flag(Permission::VoiceMute))
                     .WithArgs(2)
                     .OnExecute(HandleVoiceMute)
                     .Build());

    mgr.Register(CommandBuilder("voice_unmute")
                     .WithAliases({"vunmute", "unmute"})
                     .WithDescription("Lift an active voice mute on the target.")
                     .WithUsage("!voice_unmute <target>")
                     .RequirePermission(Flag(Permission::VoiceMute))
                     .WithArgs(1, 1)
                     .OnExecute(HandleVoiceUnmute)
                     .Build());

    mgr.Register(CommandBuilder("text_mute")
                     .WithAliases({"tmute", "gag"})
                     .WithDescription(
                         "Text-mute (chat-block) a player. Duration: minutes or 30s/5m/2h/7d; 0/'perm' = permanent.")
                     .WithUsage("!text_mute <target> <duration> [reason]")
                     .RequirePermission(Flag(Permission::TextMute))
                     .WithArgs(2)
                     .OnExecute(HandleTextMute)
                     .Build());

    mgr.Register(CommandBuilder("text_unmute")
                     .WithAliases({"tunmute", "ungag"})
                     .WithDescription("Lift an active text mute on the target.")
                     .WithUsage("!text_unmute <target>")
                     .RequirePermission(Flag(Permission::TextMute))
                     .WithArgs(1, 1)
                     .OnExecute(HandleTextUnmute)
                     .Build());

    mgr.Register(CommandBuilder("warn")
                     .WithDescription("Issue a warning. Auto-escalates to a ban once the threshold is reached.")
                     .WithUsage("!warn <target> [reason]")
                     .RequirePermission(Flag(Permission::Warn))
                     .WithArgs(1)
                     .OnExecute(HandleWarn)
                     .Build());
}

}  // namespace AdminSystem::Commands
