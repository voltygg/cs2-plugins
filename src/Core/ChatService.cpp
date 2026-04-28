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

void ChatService::BroadcastPunishment(std::string_view action, std::string_view adminName, std::string_view targetName,
                                      std::string_view reason, int64_t durationSec)
{
    const auto& cfg = ConfigManager::Instance().GetChatConfig();
    if (!cfg.BroadcastPunishments)
        return;

    // Only ban/mute/gag carry a duration; kick/warn/un* are instantaneous and shouldn't show "(permanent)".
    bool isTimedAction = (action == "banned" || action == "muted" || action == "gagged");
    std::string durationSuffix;
    if (isTimedAction)
    {
        std::string duration = (durationSec > 0) ? TimeUtils::FormatDuration(durationSec) : "permanent";
        durationSuffix = std::format(" ({})", duration);
    }

    // [ADMIN] {admin} {action} {target} for {reason}{durationSuffix}
    auto line =
        std::format("{}{} {}{}{} {}{}{} {} for {}{}{}{}", ChatColors::Green, cfg.FallbackPrefix, ChatColors::Default,
                    adminName, ChatColors::Default, ChatColors::Red, action, ChatColors::Default, targetName,
                    ChatColors::Olive, reason, ChatColors::Default, durationSuffix);
    Chat::PrintAll(line);
}

void ChatService::BroadcastAction(const std::string& translationKey, std::string_view adminName,
                                  std::string_view targetName)
{
    const auto& cfg = ConfigManager::Instance().GetChatConfig();
    if (!cfg.BroadcastPunishments)
        return;

    auto verb = Translations::Instance().Get(translationKey);
    if (verb.empty())
        verb = translationKey;  // Fallback: render the key literally so a missing translation is obvious.

    std::string line;
    if (targetName.empty())
        line = std::format("{}{} {}{}{} {}{}", ChatColors::Green, cfg.FallbackPrefix, ChatColors::Default, adminName,
                           ChatColors::Default, ChatColors::Olive, verb);
    else
        line = std::format("{}{} {}{}{} {}{}{} {}", ChatColors::Green, cfg.FallbackPrefix, ChatColors::Default,
                           adminName, ChatColors::Default, ChatColors::Olive, verb, ChatColors::Default, targetName);
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
        // anyway). Refine when CS2Kit gains a stable team accessor on Player.
        Chat::PrintAll(line);
    }
    else
    {
        Chat::PrintAll(line);
    }
}

}  // namespace AdminSystem::Core
