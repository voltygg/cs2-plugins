#pragma once

#include <cstdint>
#include <string>

namespace database {

/**
 * @brief Warning entity representing a player warning
 */
struct Warning
{
    int64_t id = 0;               // Database ID
    int64_t target_steam_id = 0;  // Warned player's SteamID64
    std::string target_name;      // Warned player's name
    int64_t admin_steam_id = 0;   // Admin who issued the warning (0 = console)
    std::string admin_name;       // Admin's name
    std::string reason;           // Warning reason
    int64_t created_at = 0;       // Warning creation timestamp
    bool is_active = true;        // Is warning currently active
    int64_t expires_at = 0;       // Warning expiration (for decay systems)
};

} // namespace database
