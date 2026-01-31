#pragma once

#include <cstdint>
#include <string>

namespace database {

/**
 * @brief Player entity representing a player's profile
 */
struct Player
{
    int64_t id = 0;               // Database ID
    int64_t steam_id = 0;         // SteamID64
    std::string name;             // Last known name
    std::string ip_address;       // Last known IP address
    int64_t first_seen = 0;       // First connection timestamp
    int64_t last_seen = 0;        // Last connection timestamp
    int32_t total_connections = 0; // Total times connected
    int64_t total_playtime = 0;   // Total playtime in seconds
};

} // namespace database
