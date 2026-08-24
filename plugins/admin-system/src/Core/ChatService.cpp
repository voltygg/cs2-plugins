#include "ChatService.hpp"

#include "Config.hpp"

#include <VoltMod/Core/ChatColors.hpp>
#include <VoltMod/Core/StringUtils.hpp>
#include <VoltMod/Core/TimeUtils.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Runtime.hpp>
#include <VoltMod/Sdk/UserMessage.hpp>
#include <format>

namespace AdminSystem::Core
{

using namespace VoltMod::Core;

namespace
{

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
    _rt.Messages.Reply(slot, message);
}

void ChatService::ReplyLink(int slot, std::string_view label, std::string_view url)
{
    _rt.Messages.Reply(slot, label);
    _rt.Messages.Reply(slot, std::format("{}{}", ChatColors::Olive, url));
}

void ChatService::NoPermission(int slot)
{
    auto msg = std::format("{}{}", ChatColors::Red, _rt.Translations.Get("cmd.noPermission", slot));
    _rt.Messages.Reply(slot, msg);
}

void ChatService::BroadcastPunishment(std::string_view action, std::string_view adminName, std::string_view targetName,
                                      std::string_view reason, int64_t durationSec)
{
    const auto& cfg = _config.GetChat();
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
    _rt.Messages.Broadcast(line);
}

void ChatService::BroadcastAction(const std::string& translationKey, std::string_view adminName,
                                  std::string_view targetName)
{
    const auto& cfg = _config.GetChat();
    if (!cfg.broadcastPunishments)
        return;

    AdminLineStyle style{.Prefix = cfg.fallbackPrefix};
    auto phrase = BroadcastPhrase(translationKey);
    _rt.Messages.Broadcast(targetName.empty() ? FormatAdminLine(style, adminName, phrase)
                                              : FormatAdminLine(style, adminName, phrase, targetName));
}

void ChatService::BroadcastAction(const std::string& translationKey, std::string_view adminName,
                                  const std::map<std::string, std::string>& nameTokens)
{
    const auto& cfg = _config.GetChat();
    if (!cfg.broadcastPunishments)
        return;

    _rt.Messages.Broadcast(FormatAdminLine(AdminLineStyle{.Prefix = cfg.fallbackPrefix}, adminName,
                                           BroadcastPhrase(translationKey), nameTokens));
}

std::string ChatService::BroadcastPhrase(const std::string& translationKey) const
{
    auto phrase = _rt.Translations.Get(translationKey);
    return phrase.empty() ? translationKey : phrase;  // Render a missing translation's key literally.
}

}  // namespace AdminSystem::Core
