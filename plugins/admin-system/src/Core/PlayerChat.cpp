#include "PlayerChat.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "ChatService.hpp"
#include "Config.hpp"

#include <CS2Kit/Commands/CommandManager.hpp>
#include <CS2Kit/Core/ChatColors.hpp>
#include <CS2Kit/Core/TimeUtils.hpp>
#include <CS2Kit/Core/Translations.hpp>
#include <CS2Kit/Players/Player.hpp>
#include <CS2Kit/Runtime.hpp>
#include <CS2Kit/Sdk/ChatInputCapture.hpp>
#include <CS2Kit/Sdk/UserMessage.hpp>
#include <format>
#include <string>

namespace AdminSystem::Core
{

using namespace CS2Kit::Players;
using namespace CS2Kit::Core;

namespace
{

/** Localized expiry suffix for mute notices addressed to @p slot. */
std::string MuteExpiryText(CS2Kit::Translations& tr, int64_t expiresAt, int slot)
{
    return TimeUtils::FormatExpiry(expiresAt, TimeUtils::Now(), tr.Get("muteNotice.permanent", slot),
                                   tr.Get("muteNotice.expiresIn", slot));
}

}  // namespace

template <class TMute>
void PlayerChat::ReplyMuteNotice(int slot, const char* noticeKey, const std::optional<TMute>& mute)
{
    auto& tr = _rt.Translations;
    if (!mute)
    {
        _chat.Reply(slot, std::format("{}{}", ChatColors::Red, tr.Get(noticeKey, slot)));
        return;
    }

    _chat.Reply(slot, std::format("{}{}{} {}{}", ChatColors::Red, tr.Get(noticeKey, slot), ChatColors::Default,
                                  ChatColors::Olive, MuteExpiryText(tr, mute->ExpiresAt, slot)));
    if (!mute->Reason.empty())
        _chat.Reply(slot, std::format("{}{}: {}{}", ChatColors::Gray, tr.Get("muteNotice.reason", slot),
                                      ChatColors::Default, mute->Reason));
}

void PlayerChat::RebroadcastAdminChat(const Player* admin, std::string_view message, bool /*teamOnly*/)
{
    if (!admin)
        return;

    auto style = _admins.GetChatStyle(admin->GetSteamID());

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
    _rt.Messages.Broadcast(line);
}

bool PlayerChat::HandleSay(Player* player, std::string_view message, bool isSayTeam)
{
    if (!player || message.empty())
        return false;

    // Menu free-text input: if a chat capture is pending for this player, the line is
    // their menu answer, not a chat message. Always supersede so it isn't broadcast.
    if (_rt.ChatInput.TryConsume(player->GetSlot(), message))
        return true;

    // Returns false for an unprefixed line and for unknown commands (e.g. "!ads"), so both
    // fall through to normal chat instead of being silently swallowed.
    if (_rt.Commands.HandleChatMessage(player, message))
        return true;

    int64_t steamId = player->GetSteamID();
    if (_punishments.IsTextMuted(steamId))
    {
        int slot = player->GetSlot();
        if (_textMuteNotice.TryAcquire(slot, TimeUtils::Now()))
            ReplyMuteNotice(slot, "muteNotice.text", _punishments.GetActiveTextMute(steamId));
        return true;
    }

    const auto& chatCfg = _config.GetChat();
    if (chatCfg.tagAdminChatMessages && _admins.IsAdmin(steamId))
    {
        RebroadcastAdminChat(player, message, isSayTeam);
        return true;
    }

    return false;
}

void PlayerChat::NotifyVoiceMuted(Player* player)
{
    if (!player)
        return;

    int slot = player->GetSlot();
    if (!_voiceMuteNotice.TryAcquire(slot, TimeUtils::Now()))
        return;

    ReplyMuteNotice(slot, "muteNotice.voice", _punishments.GetActiveVoiceMute(player->GetSteamID()));
}

}  // namespace AdminSystem::Core
