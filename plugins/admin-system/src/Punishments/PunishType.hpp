#pragma once

#include "../Core/Permissions.hpp"

#include <optional>
#include <string_view>

namespace AdminSystem::Punishments
{

/** The punishment kinds an admin can issue through the menu flow. */
enum class PunishType
{
    Kick,
    Ban,
    VoiceMute,
    TextMute,
    Warn,
};

/** Parse a punishment-type config string ("kick" | "ban" | "voiceMute" | "textMute" | "warn"). */
inline std::optional<PunishType> ParsePunishType(std::string_view text)
{
    if (text == "kick")
        return PunishType::Kick;
    if (text == "ban")
        return PunishType::Ban;
    if (text == "voiceMute")
        return PunishType::VoiceMute;
    if (text == "textMute")
        return PunishType::TextMute;
    if (text == "warn")
        return PunishType::Warn;
    return std::nullopt;
}

/** Translation key of the human-facing action name (e.g. "action.ban"). */
inline const char* ActionTranslationKey(PunishType type)
{
    switch (type)
    {
        case PunishType::Kick:
            return "action.kick";
        case PunishType::Ban:
            return "action.ban";
        case PunishType::VoiceMute:
            return "action.voiceMute";
        case PunishType::TextMute:
            return "action.textMute";
        case PunishType::Warn:
            return "action.warn";
    }
    return "action.kick";
}

/** The admin flag required to issue this punishment. */
inline Permission PermissionFor(PunishType type)
{
    switch (type)
    {
        case PunishType::Kick:
            return Permission::Kick;
        case PunishType::Ban:
            return Permission::Ban;
        case PunishType::VoiceMute:
            return Permission::VoiceMute;
        case PunishType::TextMute:
            return Permission::TextMute;
        case PunishType::Warn:
            return Permission::Warn;
    }
    return Permission::Root;
}

/** True for punishments that carry a duration (Ban/VoiceMute/TextMute). */
inline bool IsTimed(PunishType type)
{
    return type == PunishType::Ban || type == PunishType::VoiceMute || type == PunishType::TextMute;
}

}  // namespace AdminSystem::Punishments
