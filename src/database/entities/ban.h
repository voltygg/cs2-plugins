#pragma once

#include <cstdint>
#include <string>

namespace database {

/**
 * @brief Ban entity representing a player ban
 */
struct Ban
{
    int64_t id = 0;               // Database ID
    int64_t target_steam_id = 0;  // Banned player's SteamID64
    std::string target_name;      // Banned player's name
    std::string target_ip;        // Banned player's IP (for IP bans)
    int64_t admin_steam_id = 0;   // Admin who issued the ban (0 = console)
    std::string admin_name;       // Admin's name
    std::string reason;           // Ban reason
    int64_t created_at = 0;       // Ban creation timestamp
    int64_t expires_at = 0;       // Ban expiration timestamp (0 = permanent)
    int64_t duration = 0;         // Ban duration in seconds (0 = permanent)
    bool is_active = true;        // Is ban currently active
    int64_t removed_at = 0;       // When ban was removed (0 = not removed)
    int64_t removed_by = 0;       // Admin who removed the ban
    std::string removed_reason;   // Reason for removal (unban reason)

    /**
     * @brief Check if ban is permanent
     * @return true if ban has no expiration
     */
    bool IsPermanent() const { return expires_at == 0; }

    /**
     * @brief Check if ban is expired
     * @return true if ban has expired
     */
    bool IsExpired() const;
};

} // namespace database
