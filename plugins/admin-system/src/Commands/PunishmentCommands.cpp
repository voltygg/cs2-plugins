#include "../Core/Config.hpp"
#include "../Core/Managers.hpp"
#include "../Core/Permissions.hpp"
#include "../Punishments/IssuePunishment.hpp"
#include "../Punishments/PunishmentManager.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <string>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace AdminSystem::Punishments;
using CS2Kit::Registry;
using CS2Kit::Tokens;
using CS2Kit::Core::Engine;

namespace
{

/** Shared body of the kick/ban/mute/warn handlers: issue via the common entry point and
 *  reply in the caller's language. */
CommandResult Punish(CommandContext& c, PunishType type, const std::string& reason, const char* successKey,
                     const char* failedKey)
{
    // Captured before issuing: bans and kicks can drop the target immediately.
    std::string targetName = c.Target->GetName();
    if (!IssuePunishment(*c.Caller, *c.Target, type, reason, c.DurationSec))
        return c.Fail(failedKey);
    return c.Ok(successKey, {{"name", targetName}});
}

const bool _registered = [] {
    Registry<CommandSpec>::Add({
        .Name = "kick",
        .Description = "Kick a player.",
        .Usage = "!kick <target> [reason]",
        .Permission = Flag(Permission::Kick),
        .Args = {Target(), ReasonTail("reason.kickedByAdmin")},
        // Kick has no DB write, so IssuePunishment cannot fail and the failure key is never read.
        .Handler =
            [](CommandContext& c) {
                return Punish(c, PunishType::Kick, c.Reason, "cmd.kickSuccess", "cmd.kickSuccess");
            },
    });

    Registry<CommandSpec>::Add({
        .Name = "ban",
        .Description = "Ban a player. Duration: minutes (e.g. 30) or 30s/5m/2h/7d; 0/'perm' = permanent.",
        .Usage = "!ban <target> <duration> [reason]",
        .Permission = Flag(Permission::Ban),
        .Args = {Target(), Duration(), ReasonTail()},
        .Handler =
            [](CommandContext& c) {
                // The default ban reason is a config string, not a translation key.
                std::string reason = c.Reason.empty() ? App().Config.GetPunishments().defaultBanReason : c.Reason;
                return Punish(c, PunishType::Ban, reason, "cmd.banSuccess", "cmd.banFailed");
            },
    });

    Registry<CommandSpec>::Add({
        .Name = "unban",
        .Description = "Lift an active ban for the given SteamID.",
        .Usage = "!unban <steamId> [reason]",
        .Permission = Flag(Permission::Unban),
        .Args = {SteamId64("cmd.unbanUsage"), ReasonTail("reason.unbannedByAdmin")},
        .Handler =
            [](CommandContext& c) {
                bool removed = App().Punishments.RemoveBanBySteamId(c.SteamId, c.Caller->GetSteamID(), c.Reason);
                Tokens tokens{{"id", std::to_string(c.SteamId)}};
                return removed ? c.Ok("cmd.unbanSuccess", tokens) : c.Fail("cmd.unbanNoBan", tokens);
            },
    });

    Registry<CommandSpec>::Add({
        .Name = "voice_mute",
        .Aliases = {"vmute", "mute"},
        .Description = "Voice-mute a player. Duration: minutes or 30s/5m/2h/7d; 0/'perm' = permanent.",
        .Usage = "!voice_mute <target> <duration> [reason]",
        .Permission = Flag(Permission::Mute),
        .Args = {Target(), Duration(), ReasonTail("reason.voiceMutedByAdmin")},
        .Handler =
            [](CommandContext& c) {
                return Punish(c, PunishType::VoiceMute, c.Reason, "cmd.voiceMuteSuccess", "cmd.voiceMuteFailed");
            },
    });

    Registry<CommandSpec>::Add({
        .Name = "voice_unmute",
        .Aliases = {"vunmute", "unmute"},
        .Description = "Lift an active voice mute on the target.",
        .Usage = "!voice_unmute <target>",
        .Permission = Flag(Permission::Mute),
        .Args = {Target()},
        .Handler =
            [](CommandContext& c) {
                bool removed =
                    App().Punishments.RemoveVoiceMuteBySteamId(c.Target->GetSteamID(), c.Caller->GetSteamID(),
                                                               Engine().Translations.Get("reason.voiceUnmutedByAdmin"));
                Tokens tokens{{"name", c.Target->GetName()}};
                return removed ? c.Ok("cmd.voiceUnmuteSuccess", tokens) : c.Fail("cmd.voiceUnmuteNotMuted", tokens);
            },
    });

    Registry<CommandSpec>::Add({
        .Name = "text_mute",
        .Aliases = {"tmute", "gag"},
        .Description = "Text-mute (chat-block) a player. Duration: minutes or 30s/5m/2h/7d; 0/'perm' = permanent.",
        .Usage = "!text_mute <target> <duration> [reason]",
        .Permission = Flag(Permission::Mute),
        .Args = {Target(), Duration(), ReasonTail("reason.textMutedByAdmin")},
        .Handler =
            [](CommandContext& c) {
                return Punish(c, PunishType::TextMute, c.Reason, "cmd.textMuteSuccess", "cmd.textMuteFailed");
            },
    });

    Registry<CommandSpec>::Add({
        .Name = "text_unmute",
        .Aliases = {"tunmute", "ungag"},
        .Description = "Lift an active text mute on the target.",
        .Usage = "!text_unmute <target>",
        .Permission = Flag(Permission::Mute),
        .Args = {Target()},
        .Handler =
            [](CommandContext& c) {
                bool removed =
                    App().Punishments.RemoveTextMuteBySteamId(c.Target->GetSteamID(), c.Caller->GetSteamID(),
                                                              Engine().Translations.Get("reason.textUnmutedByAdmin"));
                Tokens tokens{{"name", c.Target->GetName()}};
                return removed ? c.Ok("cmd.textUnmuteSuccess", tokens) : c.Fail("cmd.textUnmuteNotMuted", tokens);
            },
    });

    Registry<CommandSpec>::Add({
        .Name = "warn",
        .Description = "Issue a warning. Auto-escalates to a ban once the threshold is reached.",
        .Usage = "!warn <target> [reason]",
        .Permission = Flag(Permission::Mute),
        .Args = {Target(), ReasonTail("reason.warnedByAdmin")},
        .Handler =
            [](CommandContext& c) {
                return Punish(c, PunishType::Warn, c.Reason, "cmd.warnSuccess", "cmd.warnFailed");
            },
    });

    return true;
}();

}  // namespace

}  // namespace AdminSystem::Commands
