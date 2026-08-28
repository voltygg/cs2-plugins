#include "../Core/App.hpp"
#include "../Core/Config.hpp"
#include "../Core/Permissions.hpp"
#include "../Punishments/IssuePunishment.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <chrono>
#include <string>
#include <string_view>

using AdminSystem::Punishments::IssuePunishment;
using AdminSystem::Punishments::PunishType;
using VoltMod::Caller;
using VoltMod::Player;
using VoltMod::Reply;
using VoltMod::Result;
using VoltMod::Tokens;

namespace Args = VoltMod::Args;

namespace AdminSystem::Commands
{

/** Shared body of the kick/ban/mute/warn handlers: issue via the common entry point and
 *  reply in the caller's language. */
static Result<Reply> Punish(App& app, const Caller& c, Player& target, PunishType type, const std::string& reason,
                            std::chrono::seconds duration, std::string_view successKey, std::string_view failedKey)
{
    // Captured before issuing: bans and kicks can drop the target immediately.
    std::string targetName = target.Name();
    if (!IssuePunishment(app, *c.Player, target, type, reason, duration.count()))
        return c.Fail(failedKey);
    return c.Ok(successKey, {{"name", targetName}});
}

void RegisterPunishmentCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Add("kick")
        .Describe("Kick a player.")
        .Permission(Flag(Permission::Kick))
        // Kick has no DB write, so IssuePunishment cannot fail and the failure key is never read.
        .Run([&app](Caller c, Args::Target t, Args::Opt<Args::Rest> why) -> Result<Reply> {
            return Punish(app, c, *t.Value, PunishType::Kick, ReasonOr(c, why, "reason.kickedByAdmin"),
                          std::chrono::seconds{0}, "cmd.kickSuccess", "cmd.kickSuccess");
        });

    commands.Add("ban")
        .Describe("Ban a player. Duration: minutes (e.g. 30) or 30s/5m/2h/7d; 0/'perm' = permanent.")
        .Permission(Flag(Permission::Ban))
        .Run([&app](Caller c, Args::Target t, Args::Duration d, Args::Opt<Args::Rest> why) -> Result<Reply> {
            // The default ban reason is a config string, not a translation key.
            std::string reason = why.Value ? why.Value->Value : app.Config.GetPunishments().defaultBanReason;
            return Punish(app, c, *t.Value, PunishType::Ban, reason, d.Value, "cmd.banSuccess", "cmd.banFailed");
        });

    commands.Add("unban")
        .Describe("Lift an active ban for the given SteamID.")
        .Permission(Flag(Permission::Unban))
        .UsageKey("cmd.unbanUsage")
        .Run([&app](Caller c, Args::SteamId id, Args::Opt<Args::Rest> why) -> Result<Reply> {
            const std::string reason = ReasonOr(c, why, "reason.unbannedByAdmin");
            bool removed = app.Punishments.RemoveBanBySteamId(id.Value, c.Player->SteamId(), reason);
            Tokens tokens{{"id", std::to_string(id.Value)}};
            return removed ? c.Ok("cmd.unbanSuccess", tokens) : c.Fail("cmd.unbanNoBan", tokens);
        });

    commands.Add("voice_mute")
        .Alias("vmute")
        .Alias("mute")
        .Describe("Voice-mute a player. Duration: minutes or 30s/5m/2h/7d; 0/'perm' = permanent.")
        .Permission(Flag(Permission::Mute))
        .Run([&app](Caller c, Args::Target t, Args::Duration d, Args::Opt<Args::Rest> why) -> Result<Reply> {
            return Punish(app, c, *t.Value, PunishType::VoiceMute, ReasonOr(c, why, "reason.voiceMutedByAdmin"),
                          d.Value, "cmd.voiceMuteSuccess", "cmd.voiceMuteFailed");
        });

    commands.Add("voice_unmute")
        .Alias("vunmute")
        .Alias("unmute")
        .Describe("Lift an active voice mute on the target.")
        .Permission(Flag(Permission::Mute))
        .Run([&app](Caller c, Args::Target t) -> Result<Reply> {
            bool removed = app.Punishments.RemoveVoiceMuteBySteamId(t.Value->SteamId(), c.Player->SteamId(),
                                                                    c.Tr.Get("reason.voiceUnmutedByAdmin"));
            Tokens tokens{{"name", t.Value->Name()}};
            return removed ? c.Ok("cmd.voiceUnmuteSuccess", tokens) : c.Fail("cmd.voiceUnmuteNotMuted", tokens);
        });

    commands.Add("text_mute")
        .Alias("tmute")
        .Alias("gag")
        .Describe("Text-mute (chat-block) a player. Duration: minutes or 30s/5m/2h/7d; 0/'perm' = permanent.")
        .Permission(Flag(Permission::Mute))
        .Run([&app](Caller c, Args::Target t, Args::Duration d, Args::Opt<Args::Rest> why) -> Result<Reply> {
            return Punish(app, c, *t.Value, PunishType::TextMute, ReasonOr(c, why, "reason.textMutedByAdmin"), d.Value,
                          "cmd.textMuteSuccess", "cmd.textMuteFailed");
        });

    commands.Add("text_unmute")
        .Alias("tunmute")
        .Alias("ungag")
        .Describe("Lift an active text mute on the target.")
        .Permission(Flag(Permission::Mute))
        .Run([&app](Caller c, Args::Target t) -> Result<Reply> {
            bool removed = app.Punishments.RemoveTextMuteBySteamId(t.Value->SteamId(), c.Player->SteamId(),
                                                                   c.Tr.Get("reason.textUnmutedByAdmin"));
            Tokens tokens{{"name", t.Value->Name()}};
            return removed ? c.Ok("cmd.textUnmuteSuccess", tokens) : c.Fail("cmd.textUnmuteNotMuted", tokens);
        });

    commands.Add("warn")
        .Describe("Issue a warning. Auto-escalates to a ban once the threshold is reached.")
        .Permission(Flag(Permission::Mute))
        .Run([&app](Caller c, Args::Target t, Args::Opt<Args::Rest> why) -> Result<Reply> {
            return Punish(app, c, *t.Value, PunishType::Warn, ReasonOr(c, why, "reason.warnedByAdmin"),
                          std::chrono::seconds{0}, "cmd.warnSuccess", "cmd.warnFailed");
        });
}

}  // namespace AdminSystem::Commands
