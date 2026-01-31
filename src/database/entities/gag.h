#pragma once

#include <cstdint>
#include <string>

namespace database {

/**
 * @brief Gag entity representing a text chat gag
 */
struct Gag
{
    int64_t id = 0;               // Database ID
    int64_t target_steam_id = 0;  // Gagged player's SteamID64
    std::string target_name;      // Gagged player's name
    int64_t admin_steam_id = 0;   // Admin who issued the gag (0 = console)
    std::string admin_name;       // Admin's name
    std::string reason;           // Gag reason
    int64_t created_at = 0;       // Gag creation timestamp
    int64_t expires_at = 0;       // Gag expiration timestamp (0 = permanent)
    int64_t duration = 0;         // Gag duration in seconds (0 = permanent)
    bool is_active = true;        // Is gag currently active
    int64_t removed_at = 0;       // When gag was removed
    int64_t removed_by = 0;       // Admin who removed the gag
    std::string removed_reason;   // Reason for removal

    bool IsPermanent() const { return expires_at == 0; }
    bool IsExpired() const;
};

} // namespace database
