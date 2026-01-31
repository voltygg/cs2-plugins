#pragma once

#include <cstdint>
#include <string>

namespace database {

/**
 * @brief Mute entity representing a voice communication mute
 */
struct Mute
{
    int64_t id = 0;               // Database ID
    int64_t target_steam_id = 0;  // Muted player's SteamID64
    std::string target_name;      // Muted player's name
    int64_t admin_steam_id = 0;   // Admin who issued the mute (0 = console)
    std::string admin_name;       // Admin's name
    std::string reason;           // Mute reason
    int64_t created_at = 0;       // Mute creation timestamp
    int64_t expires_at = 0;       // Mute expiration timestamp (0 = permanent)
    int64_t duration = 0;         // Mute duration in seconds (0 = permanent)
    bool is_active = true;        // Is mute currently active
    int64_t removed_at = 0;       // When mute was removed
    int64_t removed_by = 0;       // Admin who removed the mute
    std::string removed_reason;   // Reason for removal

    bool IsPermanent() const { return expires_at == 0; }
    bool IsExpired() const;
};

} // namespace database
