#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace database {

/**
 * @brief Admin group entity representing a permission group
 */
struct AdminGroup
{
    int64_t id = 0;               // Database ID
    std::string name;             // Group name (unique)
    std::string flags;            // Permission flags
    int32_t immunity = 0;         // Immunity level
    std::vector<std::string> inherits; // Parent groups to inherit from
    int64_t created_at = 0;       // Creation timestamp
    int64_t updated_at = 0;       // Last update timestamp

    /**
     * @brief Check if group has a specific permission flag
     * @param flag Permission flag to check (single character)
     * @return true if group has the flag
     */
    bool HasFlag(char flag) const;
};

} // namespace database
