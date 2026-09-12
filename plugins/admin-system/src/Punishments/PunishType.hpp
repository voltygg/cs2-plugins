#pragma once

#include "../Core/Permissions.hpp"

#include <VoltMod/Core/EnumNames.hpp>
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

/** Parse a punishment-type config string ("kick" | "ban" | "voiceMute" | "textMute" | "warn"),
 *  case-insensitively. */
inline std::optional<PunishType> ParsePunishType(std::string_view text)
{
    return VoltMod::Parse<PunishType>(text);
}

/** Translation key of the human-facing action name (e.g. "action.ban"). */
inline std::string_view ActionTranslationKey(PunishType type)
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

/** The value stored in `admin_activity.action` and in `punishments.kind`. */
inline std::string_view AuditActionName(PunishType type)
{
    switch (type)
    {
    case PunishType::Kick:
        return "kick";
    case PunishType::Ban:
        return "ban";
    case PunishType::VoiceMute:
        return "voice_mute";
    case PunishType::TextMute:
        return "text_mute";
    case PunishType::Warn:
        return "warn";
    }
    return "kick";
}

/** Read one of those columns back; nullopt for a value this build does not know. */
inline std::optional<PunishType> ParseAuditAction(std::string_view name)
{
    for (PunishType type : VoltMod::EnumValues<PunishType>())
    {
        if (AuditActionName(type) == name)
            return type;
    }
    return std::nullopt;
}

/** Translation key of the default reason for lifting this punishment; empty for a kind that cannot
 *  be lifted. */
inline std::string_view LiftReasonKey(PunishType type)
{
    switch (type)
    {
    case PunishType::Ban:
        return "reason.unbannedByAdmin";
    case PunishType::VoiceMute:
        return "reason.voiceUnmutedByAdmin";
    case PunishType::TextMute:
        return "reason.textUnmutedByAdmin";
    case PunishType::Kick:
    case PunishType::Warn:
        break;
    }
    return {};
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
    case PunishType::TextMute:
    case PunishType::Warn:
        return Permission::Mute;
    }
    return Permission::Root;
}

/** True for punishments that carry a duration (Ban/VoiceMute/TextMute). */
inline bool IsTimed(PunishType type)
{
    return type == PunishType::Ban || type == PunishType::VoiceMute || type == PunishType::TextMute;
}

}  // namespace AdminSystem::Punishments
