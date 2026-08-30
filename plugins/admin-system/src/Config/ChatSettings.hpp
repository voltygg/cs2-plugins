#pragma once

#include <string>

namespace AdminSystem::Config
{
struct ChatSettings
{
    bool broadcastPunishments = true;

    bool tagAdminChatMessages = true;

    /** Prefix used when an admin doesn't belong to any group with a prefix set. */
    std::string fallbackPrefix = "[ADMIN]";
    std::string fallbackPrefixColor = "red";
    std::string fallbackNameColor = "default";
    std::string fallbackMessageColor = "default";
};

}  // namespace AdminSystem::Config
