#include "../Core/App.hpp"
#include "../Core/Config.hpp"
#include "../Core/Permissions.hpp"
#include "../Punishments/IssuePunishment.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "Commands.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Translations.hpp>
#include <CS2Kit/Runtime.hpp>
#include <string>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace AdminSystem::Punishments;
using CS2Kit::Tokens;

namespace
{

/** Shared body of the kick/ban/mute/warn handlers: issue via the common entry point and
 *  reply in the caller's language. */
CommandResult Punish(App& app, CommandContext& c, PunishType type, const std::string& reason, const char* successKey,
                     const char* failedKey)
{
    // Captured before issuing: bans and kicks can drop the target immediately.
    std::string targetName = c.Target().GetName();
    if (!IssuePunishment(app, *c.Caller, c.Target(), type, reason, c.Duration().value_or(0)))
        return c.Fail(failedKey);
    return c.Ok(successKey, {{"name", targetName}});
}

}  // namespace

void RegisterPunishmentCommands(CS2Kit::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "kick",
        .Description = "Kick a player.",
        .Permission = Flag(Permission::Kick),
        .Args = {Target(), ReasonTail("reason.kickedByAdmin")},
        // Kick has no DB write, so IssuePunishment cannot fail and the failure key is never read.
        .Handler =
            [&app](CommandContext& c) {
                return Punish(app, c, PunishType::Kick, c.Reason, "cmd.kickSuccess", "cmd.kickSuccess");
            },
    });

    commands.Register({
        .Name = "ban",
        .Description = "Ban a player. Duration: minutes (e.g. 30) or 30s/5m/2h/7d; 0/'perm' = permanent.",
        .Permission = Flag(Permission::Ban),
        .Args = {Target(), Duration(), ReasonTail()},
        .Handler =
            [&app](CommandContext& c) {
                // The default ban reason is a config string, not a translation key.
                std::string reason = c.Reason.empty() ? app.Config.GetPunishments().defaultBanReason : c.Reason;
                return Punish(app, c, PunishType::Ban, reason, "cmd.banSuccess", "cmd.banFailed");
            },
    });

    commands.Register({
        .Name = "unban",
        .Description = "Lift an active ban for the given SteamID.",
        .Permission = Flag(Permission::Unban),
        .Args = {SteamId64("cmd.unbanUsage"), ReasonTail("reason.unbannedByAdmin")},
        .Handler =
            [&app](CommandContext& c) {
                bool removed = app.Punishments.RemoveBanBySteamId(c.SteamId, c.Caller->GetSteamID(), c.Reason);
                Tokens tokens{{"id", std::to_string(c.SteamId)}};
                return removed ? c.Ok("cmd.unbanSuccess", tokens) : c.Fail("cmd.unbanNoBan", tokens);
            },
    });

    commands.Register({
        .Name = "voice_mute",
        .Aliases = {"vmute", "mute"},
        .Description = "Voice-mute a player. Duration: minutes or 30s/5m/2h/7d; 0/'perm' = permanent.",
        .Permission = Flag(Permission::Mute),
        .Args = {Target(), Duration(), ReasonTail("reason.voiceMutedByAdmin")},
        .Handler =
            [&app](CommandContext& c) {
                return Punish(app, c, PunishType::VoiceMute, c.Reason, "cmd.voiceMuteSuccess", "cmd.voiceMuteFailed");
            },
    });

    commands.Register({
        .Name = "voice_unmute",
        .Aliases = {"vunmute", "unmute"},
        .Description = "Lift an active voice mute on the target.",
        .Permission = Flag(Permission::Mute),
        .Args = {Target()},
        .Handler =
            [&app](CommandContext& c) {
                bool removed = app.Punishments.RemoveVoiceMuteBySteamId(
                    c.Target().GetSteamID(), c.Caller->GetSteamID(),
                    app.Runtime.Translations.Get("reason.voiceUnmutedByAdmin"));
                Tokens tokens{{"name", c.Target().GetName()}};
                return removed ? c.Ok("cmd.voiceUnmuteSuccess", tokens) : c.Fail("cmd.voiceUnmuteNotMuted", tokens);
            },
    });

    commands.Register({
        .Name = "text_mute",
        .Aliases = {"tmute", "gag"},
        .Description = "Text-mute (chat-block) a player. Duration: minutes or 30s/5m/2h/7d; 0/'perm' = permanent.",
        .Permission = Flag(Permission::Mute),
        .Args = {Target(), Duration(), ReasonTail("reason.textMutedByAdmin")},
        .Handler =
            [&app](CommandContext& c) {
                return Punish(app, c, PunishType::TextMute, c.Reason, "cmd.textMuteSuccess", "cmd.textMuteFailed");
            },
    });

    commands.Register({
        .Name = "text_unmute",
        .Aliases = {"tunmute", "ungag"},
        .Description = "Lift an active text mute on the target.",
        .Permission = Flag(Permission::Mute),
        .Args = {Target()},
        .Handler =
            [&app](CommandContext& c) {
                bool removed =
                    app.Punishments.RemoveTextMuteBySteamId(c.Target().GetSteamID(), c.Caller->GetSteamID(),
                                                            app.Runtime.Translations.Get("reason.textUnmutedByAdmin"));
                Tokens tokens{{"name", c.Target().GetName()}};
                return removed ? c.Ok("cmd.textUnmuteSuccess", tokens) : c.Fail("cmd.textUnmuteNotMuted", tokens);
            },
    });

    commands.Register({
        .Name = "warn",
        .Description = "Issue a warning. Auto-escalates to a ban once the threshold is reached.",
        .Permission = Flag(Permission::Mute),
        .Args = {Target(), ReasonTail("reason.warnedByAdmin")},
        .Handler =
            [&app](CommandContext& c) {
                return Punish(app, c, PunishType::Warn, c.Reason, "cmd.warnSuccess", "cmd.warnFailed");
            },
    });
}

}  // namespace AdminSystem::Commands
