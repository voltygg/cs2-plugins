#pragma once

#include "../entities/ban.h"
#include <pqxx/pqxx>
#include <optional>
#include <vector>

namespace database {

/**
 * @brief Repository for Ban entities
 */
class BanRepository
{
public:
    /**
     * @brief Find active ban by SteamID
     * @param steam_id SteamID64 to search for
     * @return Ban if found and active
     */
    std::optional<Ban> FindActiveBySteamID(int64_t steam_id);

    /**
     * @brief Find active ban by IP address
     * @param ip_address IP address to search for
     * @return Ban if found and active
     */
    std::optional<Ban> FindActiveByIP(const std::string& ip_address);

    /**
     * @brief Get all active bans
     * @return Vector of active bans
     */
    std::vector<Ban> FindAllActive();

    /**
     * @brief Create a new ban
     * @param ban Ban entity to create
     * @return true if successful
     */
    bool Create(const Ban& ban);

    /**
     * @brief Update an existing ban
     * @param ban Ban entity to update
     * @return true if successful
     */
    bool Update(const Ban& ban);

    /**
     * @brief Remove (deactivate) a ban
     * @param id Ban ID to remove
     * @param removed_by Admin SteamID who removed it
     * @param reason Reason for removal
     * @return true if successful
     */
    bool Remove(int64_t id, int64_t removed_by, const std::string& reason);

    /**
     * @brief Expire old bans (set is_active = false for expired bans)
     * @return Number of bans expired
     */
    int ExpireOldBans();

    /**
     * @brief Get ban history for a player
     * @param steam_id SteamID64 to search for
     * @param limit Maximum number of results
     * @return Vector of bans
     */
    std::vector<Ban> GetHistory(int64_t steam_id, int limit = 10);

private:
    Ban ParseRow(const pqxx::row& row);
};

} // namespace database
