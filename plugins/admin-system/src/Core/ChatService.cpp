#include "ChatService.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "Config.hpp"
#include "Managers.hpp"

#include <CS2Kit/Commands/CommandManager.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/ChatInputCapture.hpp>
#include <CS2Kit/Sdk/UserMessage.hpp>
#include <CS2Kit/Utils/ChatColors.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

using CS2Kit::Core::Engine;

namespace AdminSystem::Core
{

using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;
using AdminSystem::Admin::AdminManager;
using AdminSystem::Punishments::PunishmentManager;
using CS2Kit::Commands::CommandManager;

namespace
{

/** Localized expiry suffix for mute notices addressed to @p slot. */
std::string MuteExpiryText(int64_t expiresAt, int slot)
{
    auto& tr = Engine().Translations;
    return TimeUtils::FormatExpiry(expiresAt, TimeUtils::Now(), tr.Get("muteNotice.permanent", slot),
                                   tr.Get("muteNotice.expiresIn", slot));
}

/** Layout for "{prefix} {actor} {phrase}" action lines. */
struct AdminLineStyle
{
    std::string Prefix;
    std::string_view PrefixColor = ChatColors::Green;
    std::string_view NameColor = ChatColors::Default;
    std::string_view PhraseColor = ChatColors::Olive;
};

/** "{prefix} {actor} {phrase}", e.g. "[ADMIN] Bob went stealth". */
std::string FormatAdminLine(const AdminLineStyle& style, std::string_view actorName, std::string_view phrase)
{
    // {PrefixColor}{prefix} {NameColor}{actor}{Default} {PhraseColor}{phrase}
    return std::format("{}{} {}{}{} {}{}", style.PrefixColor, style.Prefix, style.NameColor, actorName,
                       ChatColors::Default, style.PhraseColor, phrase);
}

/** Single-target variant: "{prefix} {actor} {phrase} {target}", e.g. "[ADMIN] Bob slapped Alice". */
std::string FormatAdminLine(const AdminLineStyle& style, std::string_view actorName, std::string_view phrase,
                            std::string_view targetName)
{
    return std::format("{}{} {}", FormatAdminLine(style, actorName, phrase), ChatColors::Default, targetName);
}

/** Token variant for multi-target phrases, e.g. "swapped {a} and {b}": each mapped name is
 *  substituted into `phraseTemplate` wrapped in the name color. */
std::string FormatAdminLine(const AdminLineStyle& style, std::string_view actorName, std::string_view phraseTemplate,
                            const std::map<std::string, std::string>& nameTokens)
{
    // Names sit inside the colored phrase, so wrap each in the name color before substituting.
    std::map<std::string, std::string> colored;
    for (const auto& [token, name] : nameTokens)
        colored.emplace(token, std::format("{}{}{}", style.NameColor, name, style.PhraseColor));

    return FormatAdminLine(style, actorName, StringUtils::SubstituteTokens(std::string(phraseTemplate), colored));
}

}  // namespace

void ChatService::Reply(int slot, std::string_view message)
{
    Engine().Messages.Reply(slot, message);
}

void ChatService::ReplyLink(int slot, std::string_view label, std::string_view url)
{
    Engine().Messages.Reply(slot, label);
    Engine().Messages.Reply(slot, std::format("{}{}", ChatColors::Olive, url));
}

void ChatService::NoPermission(int slot)
{
    auto msg = std::format("{}{}", ChatColors::Red, Engine().Translations.Get("common.noPermission", slot));
    Engine().Messages.Reply(slot, msg);
}

void ChatService::BroadcastPunishment(std::string_view action, std::string_view adminName, std::string_view targetName,
                                      std::string_view reason, int64_t durationSec)
{
    const auto& cfg = App().Config.GetChat();
    if (!cfg.broadcastPunishments)
        return;

    // Only ban/voice-mute/text-mute carry a duration; kick/warn/un* are instantaneous.
    bool isTimedAction = (action == "banned" || action == "voice-muted" || action == "text-muted");
    std::string durationSuffix;
    if (isTimedAction)
    {
        std::string duration = (durationSec > 0) ? TimeUtils::FormatDuration(durationSec) : "permanent";
        durationSuffix = std::format(" ({})", duration);
    }

    // [ADMIN] {admin} {action} {target} for {reason}{durationSuffix}
    auto line =
        std::format("{}{} {}{}{} {}{}{} {} for {}{}{}{}", ChatColors::Green, cfg.fallbackPrefix, ChatColors::Default,
                    adminName, ChatColors::Default, ChatColors::Red, action, ChatColors::Default, targetName,
                    ChatColors::Olive, reason, ChatColors::Default, durationSuffix);
    Engine().Messages.Broadcast(line);
}

void ChatService::BroadcastAction(const std::string& translationKey, std::string_view adminName,
                                  std::string_view targetName)
{
    const auto& cfg = App().Config.GetChat();
    if (!cfg.broadcastPunishments)
        return;

    AdminLineStyle style{.Prefix = cfg.fallbackPrefix};
    auto phrase = BroadcastPhrase(translationKey);
    Engine().Messages.Broadcast(targetName.empty() ? FormatAdminLine(style, adminName, phrase)
                                                   : FormatAdminLine(style, adminName, phrase, targetName));
}

void ChatService::BroadcastAction(const std::string& translationKey, std::string_view adminName,
                                  const std::map<std::string, std::string>& nameTokens)
{
    const auto& cfg = App().Config.GetChat();
    if (!cfg.broadcastPunishments)
        return;

    Engine().Messages.Broadcast(FormatAdminLine(AdminLineStyle{.Prefix = cfg.fallbackPrefix}, adminName,
                                                BroadcastPhrase(translationKey), nameTokens));
}

std::string ChatService::BroadcastPhrase(const std::string& translationKey) const
{
    auto phrase = Engine().Translations.Get(translationKey);
    return phrase.empty() ? translationKey : phrase;  // Render a missing translation's key literally.
}

void ChatService::RebroadcastAdminChat(const Player* admin, std::string_view message, bool /*teamOnly*/)
{
    if (!admin)
        return;

    auto style = App().Admins.GetChatStyle(admin->GetSteamID());

    auto prefixColor = ChatColors::ParseNamed(style.PrefixColor);

    // Hiding the prefix is an incognito signal: default the name and message colors too, so
    // custom colors don't still mark the admin as staff. Render-only - the saved colors return
    // when the prefix is re-enabled.
    auto nameColor = style.DisplayPrefix ? ChatColors::ParseNamed(style.NameColor) : ChatColors::Default;
    auto messageColor = style.DisplayPrefix ? ChatColors::ParseNamed(style.MessageColor) : ChatColors::Default;

    // {prefixColor}{prefix} {nameColor}{name}{Default}: {messageColor}{message}
    std::string line;
    if (style.HasPrefix())
        line = std::format("{}{} {}{}{}: {}{}", prefixColor, style.Prefix, nameColor, admin->GetName(),
                           ChatColors::Default, messageColor, message);
    else
        line = std::format("{}{}{}: {}{}", nameColor, admin->GetName(), ChatColors::Default, messageColor, message);

    // Team-only filtering isn't implemented yet (no stable team accessor on Player), so admin chat
    // currently broadcasts to everyone regardless of say vs say_team.
    Engine().Messages.Broadcast(line);
}

bool ChatService::HandleSay(Player* player, std::string_view message, bool isSayTeam)
{
    if (!player || message.empty())
        return false;

    // Menu free-text input: if a chat capture is pending for this player, the line is
    // their menu answer, not a chat message. Always supersede so it isn't broadcast.
    if (Engine().ChatInput.TryConsume(player->GetSlot(), message))
        return true;

    bool isCommand = (message.front() == '!' || message.front() == '.');

    // Try to dispatch as a registered command. Returns false for unknown commands
    // (e.g. "!ads") so they fall through to normal chat instead of being silently swallowed.
    if (isCommand && Engine().Commands.HandleChatMessage(player, std::string(message)))
        return true;

    int64_t steamId = player->GetSteamID();
    if (App().Punishments.IsTextMuted(steamId))
    {
        int slot = player->GetSlot();
        if (_textMuteNotice.TryAcquire(slot, TimeUtils::Now()))
        {
            auto mute = App().Punishments.GetActiveTextMute(steamId);
            auto& tr = Engine().Translations;
            if (mute)
            {
                Engine().Messages.Reply(
                    slot, std::format("{}{}{} {}{}", ChatColors::Red, tr.Get("muteNotice.text", slot),
                                      ChatColors::Default, ChatColors::Olive, MuteExpiryText(mute->ExpiresAt, slot)));
                if (!mute->Reason.empty())
                    Engine().Messages.Reply(
                        slot, std::format("{}{}: {}{}", ChatColors::Gray, tr.Get("muteNotice.reason", slot),
                                          ChatColors::Default, mute->Reason));
            }
            else
            {
                Engine().Messages.Reply(slot, std::format("{}{}", ChatColors::Red, tr.Get("muteNotice.text", slot)));
            }
        }
        return true;
    }

    const auto& chatCfg = App().Config.GetChat();
    if (chatCfg.tagAdminChatMessages && App().Admins.IsAdmin(steamId))
    {
        RebroadcastAdminChat(player, message, isSayTeam);
        return true;
    }

    return false;
}

void ChatService::NotifyVoiceMuted(Player* player)
{
    if (!player)
        return;

    int slot = player->GetSlot();
    if (!_voiceMuteNotice.TryAcquire(slot, TimeUtils::Now()))
        return;

    auto mute = App().Punishments.GetActiveVoiceMute(player->GetSteamID());
    auto& tr = Engine().Translations;
    if (mute)
    {
        Engine().Messages.Reply(
            slot, std::format("{}{}{} {}{}", ChatColors::Red, tr.Get("muteNotice.voice", slot), ChatColors::Default,
                              ChatColors::Olive, MuteExpiryText(mute->ExpiresAt, slot)));
        if (!mute->Reason.empty())
            Engine().Messages.Reply(slot, std::format("{}{}: {}{}", ChatColors::Gray, tr.Get("muteNotice.reason", slot),
                                                      ChatColors::Default, mute->Reason));
    }
    else
    {
        Engine().Messages.Reply(slot, std::format("{}{}", ChatColors::Red, tr.Get("muteNotice.voice", slot)));
    }
}

}  // namespace AdminSystem::Core
