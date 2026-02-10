#pragma once

#include "../entities/admin.h"
#include "../entities/admin_group.h"
#include <pqxx/pqxx>
#include <optional>
#include <vector>

namespace database {

/**
 * @brief Repository for Admin entities
 */
class AdminRepository
{
public:
    /**
     * @brief Find admin by SteamID
     * @param steam_id SteamID64 to search for
     * @return Admin if found
     */
    std::optional<Admin> FindBySteamID(int64_t steam_id);

    /**
     * @brief Get all admins
     * @return Vector of all admins
     */
    std::vector<Admin> FindAll();

    /**
     * @brief Delete an admin
     * @param steam_id SteamID64 to delete
     * @return true if successful
     */
    bool Delete(int64_t steam_id);

private:
    Admin ParseRow(const pqxx::row& row);
};

/**
 * @brief Repository for AdminGroup entities
 */
class AdminGroupRepository
{
public:
    /**
     * @brief Find group by name
     * @param name Group name to search for
     * @return AdminGroup if found
     */
    std::optional<AdminGroup> FindByName(const std::string& name);

    /**
     * @brief Get all groups
     * @return Vector of all groups
     */
    std::vector<AdminGroup> FindAll();

    /**
     * @brief Delete a group
     * @param name Group name to delete
     * @return true if successful
     */
    bool Delete(const std::string& name);

private:
    AdminGroup ParseRow(const pqxx::row& row);
};

} // namespace database
