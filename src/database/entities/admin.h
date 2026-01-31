#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace database {

/**
 * @brief Admin entity representing an administrator
 */
struct Admin
{
    int64_t id = 0;               // Database ID
    int64_t steam_id = 0;         // SteamID64
    std::string name;             // Admin name
    std::vector<std::string> groups; // Group names
    std::string flags;            // Permission flags (single letter each)
    int32_t immunity = 0;         // Immunity level (higher = more immune)
    int64_t created_at = 0;       // Creation timestamp
    int64_t updated_at = 0;       // Last update timestamp

    /**
     * @brief Check if admin has a specific permission flag
     * @param flag Permission flag to check (single character)
     * @return true if admin has the flag
     */
    bool HasFlag(char flag) const;

    /**
     * @brief Check if admin has all specified flags
     * @param requiredFlags Flags to check (string of characters)
     * @return true if admin has all flags
     */
    bool HasAllFlags(const std::string& requiredFlags) const;

    /**
     * @brief Check if admin has any of the specified flags
     * @param anyFlags Flags to check (string of characters)
     * @return true if admin has any of the flags
     */
    bool HasAnyFlag(const std::string& anyFlags) const;

    /**
     * @brief Check if admin has root access (z flag)
     * @return true if admin has root access
     */
    bool IsRoot() const;
};

/**
 * @brief Permission flags
 *
 * Standard flags:
 * - a: Generic admin (required for most commands)
 * - b: Ban/unban players
 * - c: Kick players
 * - d: Slay/harm players
 * - e: Change map
 * - f: Change cvars
 * - g: Manage bans
 * - h: Password access
 * - i: Chat management (mute, gag)
 * - j: Vote management
 * - k: Reserved slot access
 * - l: Rcon access
 * - m: Custom vote access
 * - n: Reserved (custom)
 * - o: Custom commands
 * - p: Custom commands (2)
 * - q: Custom commands (3)
 * - r: Custom commands (4)
 * - s: Custom commands (5)
 * - t: Custom commands (6)
 * - z: Root access (all permissions)
 */

} // namespace database
