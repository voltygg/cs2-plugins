#include "ChatService.hpp"

#include "../Admin/AdminManager.hpp"
#include "Config.hpp"

#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/Chat.hpp>
#include <CS2Kit/Utils/ChatColors.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>

#include <format>

namespace AdminSystem::Core
{

using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;
using AdminSystem::Admin::AdminManager;

void ChatService::Reply(int slot, std::string_view message)
{
    Chat::Print(slot, message);
}

void ChatService::NoPermission(int slot)
{
    auto msg = std::format("{}{}", ChatColors::Red, Translations::Instance().Get("noPermission"));
    Chat::Print(slot, msg);
}

void ChatService::BroadcastPunishment(std::string_view action, std::string_view adminName,
                                      std::string_view targetName, std::string_view reason, int64_t durationSec)
{
    const auto& cfg = ConfigManager::Instance().GetChatConfig();
    if (!cfg.BroadcastPunishments)
        return;

    std::string duration = (durationSec > 0) ? TimeUtils::FormatDuration(durationSec) : "permanent";

    // [ADMIN] {admin} {action} {target} for {reason} ({duration})
    auto line = std::format("{}{} {}{}{} {}{}{} {} for {}{}{} ({})",
                            ChatColors::Green, cfg.FallbackPrefix,
                            ChatColors::Default, adminName, ChatColors::Default,
                            ChatColors::Red, action, ChatColors::Default,
                            targetName,
                            ChatColors::Olive, reason, ChatColors::Default,
                            duration);
    Chat::PrintAll(line);
}

void ChatService::RebroadcastAdminChat(const Player* admin, std::string_view message, bool teamOnly)
{
    if (!admin)
        return;

    auto style = AdminManager::Instance().GetChatStyle(admin->GetSteamID());

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
        // anyway). Refine when CS2-Kit gains a stable team accessor on Player.
        Chat::PrintAll(line);
    }
    else
    {
        Chat::PrintAll(line);
    }
}

}  // namespace AdminSystem::Core
