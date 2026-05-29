#include "ChatService.hpp"
#include "Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../Admin/AdminManager.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "ChatFormat.hpp"
#include "Config.hpp"

#include <CS2Kit/Commands/CommandManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/ChatInputCapture.hpp>
#include <CS2Kit/Utils/Chat.hpp>
#include <CS2Kit/Utils/ChatColors.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

using CS2Kit::Core::Kit;

namespace AdminSystem::Core
{

using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;
using AdminSystem::Admin::AdminManager;
using AdminSystem::Punishments::PunishmentManager;
using CS2Kit::Commands::CommandManager;

namespace
{

// Rate limit (seconds) for "you are muted" notifications. The voice hook fires per receiver
// every voice keypress, and chat-spam quickly produces dozens of say events; once per minute
// is enough to be informative without becoming the spam itself.
constexpr int64_t kMuteNoticeIntervalSec = 60;

}  // namespace

void ChatService::Reply(int slot, std::string_view message)
{
    Chat::Print(slot, message);
}

void ChatService::NoPermission(int slot)
{
    auto msg = std::format("{}{}", ChatColors::Red, Kit().Translations.Get("common.noPermission", slot));
    Chat::Print(slot, msg);
}

void ChatService::BroadcastPunishment(std::string_view action, std::string_view adminName, std::string_view targetName,
                                      std::string_view reason, int64_t durationSec)
{
    const auto& cfg = Sys().Config.GetChat();
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
    Chat::PrintAll(line);
}

void ChatService::BroadcastAction(const std::string& translationKey, std::string_view adminName,
                                  std::string_view targetName)
{
    const auto& cfg = Sys().Config.GetChat();
    if (!cfg.broadcastPunishments)
        return;

    auto verb = Kit().Translations.Get(translationKey);
    if (verb.empty())
        verb = translationKey;  // Fallback: render the key literally so a missing translation is obvious.

    std::string line;
    if (targetName.empty())
        line = std::format("{}{} {}{}{} {}{}", ChatColors::Green, cfg.fallbackPrefix, ChatColors::Default, adminName,
                           ChatColors::Default, ChatColors::Olive, verb);
    else
        line = std::format("{}{} {}{}{} {}{}{} {}", ChatColors::Green, cfg.fallbackPrefix, ChatColors::Default,
                           adminName, ChatColors::Default, ChatColors::Olive, verb, ChatColors::Default, targetName);
    Chat::PrintAll(line);
}

void ChatService::RebroadcastAdminChat(const Player* admin, std::string_view message, bool teamOnly)
{
    if (!admin)
        return;

    auto style = Sys().Admins.GetChatStyle(admin->GetSteamID());

    auto prefixColor = ChatColors::ParseNamed(style.PrefixColor);
    auto nameColor = ChatColors::ParseNamed(style.NameColor);
    auto messageColor = ChatColors::ParseNamed(style.MessageColor);

    // {prefixColor}{prefix} {nameColor}{name}{Default}: {messageColor}{message}
    std::string line;
    if (style.HasPrefix())
        line = std::format("{}{} {}{}{}: {}{}", prefixColor, style.Prefix, nameColor, admin->GetName(),
                           ChatColors::Default, messageColor, message);
    else
        line = std::format("{}{}{}: {}{}", nameColor, admin->GetName(), ChatColors::Default, messageColor, message);

    if (teamOnly)
    {
        // Team-only chat: filter to players on the admin's team. PlayerController exposes team via
        // schema; for now we broadcast to everyone (most servers run admin team chat as a notice
        // anyway). Refine when CS2Kit gains a stable team accessor on Player.
        Chat::PrintAll(line);
    }
    else
    {
        Chat::PrintAll(line);
    }
}

bool ChatService::HandleSay(Player* player, std::string_view message, bool isSayTeam)
{
    if (!player || message.empty())
        return false;

    // Menu free-text input: if a chat capture is pending for this player, the line is
    // their menu answer, not a chat message. Always supersede so it isn't broadcast.
    if (Kit().ChatInput.TryConsume(player->GetSlot(), message))
        return true;

    bool isCommand = (message.front() == '!' || message.front() == '.');

    // Try to dispatch as a registered command. Returns false for unknown commands
    // (e.g. "!ads") so they fall through to normal chat instead of being silently swallowed.
    if (isCommand && Kit().Commands.HandleChatMessage(player, std::string(message)))
        return true;

    int64_t steamId = player->GetSteamID();
    if (Sys().Punishments.IsTextMuted(steamId))
    {
        int slot = player->GetSlot();
        int64_t now = TimeUtils::Now();
        auto& last = _textMuteNoticeAt[slot];

        if (now - last >= kMuteNoticeIntervalSec)
        {
            last = now;
            auto mute = Sys().Punishments.GetActiveTextMute(steamId);
            auto& tr = Kit().Translations;
            if (mute)
            {
                Chat::Print(slot, std::format("{}{}{} {}{}", ChatColors::Red, tr.Get("muteNotice.text", slot),
                                              ChatColors::Default, ChatColors::Olive,
                                              ChatFormat::FormatExpiry(mute->ExpiresAt, slot)));
                if (!mute->Reason.empty())
                    Chat::Print(slot, std::format("{}{}: {}{}", ChatColors::Gray, tr.Get("muteNotice.reason", slot),
                                                  ChatColors::Default, mute->Reason));
            }
            else
            {
                Chat::Print(slot, std::format("{}{}", ChatColors::Red, tr.Get("muteNotice.text", slot)));
            }
        }
        return true;
    }

    const auto& chatCfg = Sys().Config.GetChat();
    if (chatCfg.tagAdminChatMessages && Sys().Admins.IsAdmin(steamId))
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
    int64_t now = TimeUtils::Now();
    auto& last = _voiceMuteNoticeAt[slot];
    if (now - last < kMuteNoticeIntervalSec)
        return;
    last = now;

    auto mute = Sys().Punishments.GetActiveVoiceMute(player->GetSteamID());
    auto& tr = Kit().Translations;
    if (mute)
    {
        Chat::Print(slot, std::format("{}{}{} {}{}", ChatColors::Red, tr.Get("muteNotice.voice", slot),
                                      ChatColors::Default, ChatColors::Olive,
                                      ChatFormat::FormatExpiry(mute->ExpiresAt, slot)));
        if (!mute->Reason.empty())
            Chat::Print(slot, std::format("{}{}: {}{}", ChatColors::Gray, tr.Get("muteNotice.reason", slot),
                                          ChatColors::Default, mute->Reason));
    }
    else
    {
        Chat::Print(slot, std::format("{}{}", ChatColors::Red, tr.Get("muteNotice.voice", slot)));
    }
}

}  // namespace AdminSystem::Core
