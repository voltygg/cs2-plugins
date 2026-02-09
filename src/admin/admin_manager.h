#pragma once

#include "../database/entities/admin.h"
#include "../database/entities/admin_group.h"
#include <unordered_map>
#include <mutex>
#include <string>

namespace admin {

/**
 * @brief AdminManager class for managing admins and permission checking
 */
class AdminManager
{
public:
    /**
     * @brief Load admins from database
     * @return true if successful
     */
    bool LoadAdmins();

    /**
     * @brief Load admin groups from database
     * @return true if successful
     */
    bool LoadGroups();

    /**
     * @brief Reload all admins and groups
     * @return true if successful
     */
    bool Reload();

    /**
     * @brief Check if player is an admin
     * @param steam_id SteamID64
     * @return true if player is admin
     */
    bool IsAdmin(int64_t steam_id);

    /**
     * @brief Get admin by SteamID
     * @param steam_id SteamID64
     * @return Pointer to admin or nullptr
     */
    const database::Admin* GetAdmin(int64_t steam_id);

    /**
     * @brief Check if admin has permission flag
     * @param steam_id SteamID64
     * @param flag Permission flag (single character)
     * @return true if admin has flag
     */
    bool HasPermission(int64_t steam_id, char flag);

    /**
     * @brief Check if admin has all specified flags
     * @param steam_id SteamID64
     * @param flags Flags to check (string of characters)
     * @return true if admin has all flags
     */
    bool HasAllPermissions(int64_t steam_id, const std::string& flags);

    /**
     * @brief Check if admin has any of the specified flags
     * @param steam_id SteamID64
     * @param flags Flags to check (string of characters)
     * @return true if admin has any flag
     */
    bool HasAnyPermission(int64_t steam_id, const std::string& flags);

    /**
     * @brief Get admin's effective immunity (including group immunity)
     * @param steam_id SteamID64
     * @return Immunity level
     */
    int GetImmunity(int64_t steam_id);

    /**
     * @brief Check if admin can target another player
     * @param admin_steam_id Admin's SteamID64
     * @param target_steam_id Target's SteamID64
     * @return true if admin can target player
     */
    bool CanTarget(int64_t admin_steam_id, int64_t target_steam_id);

    /**
     * @brief Add admin to cache (from file or runtime)
     * @param admin Admin entity
     */
    void AddAdmin(const database::Admin& admin);

    /**
     * @brief Add group to cache (from file or runtime)
     * @param group AdminGroup entity
     */
    void AddGroup(const database::AdminGroup& group);

    /**
     * @brief Remove admin from cache
     * @param steam_id SteamID64
     */
    void RemoveAdmin(int64_t steam_id);

    /**
     * @brief Get singleton instance
     * @return AdminManager instance
     */
    static AdminManager& Instance();

private:
    AdminManager() = default;
    ~AdminManager() = default;
    AdminManager(const AdminManager&) = delete;
    AdminManager& operator=(const AdminManager&) = delete;

    /**
     * @brief Resolve admin's effective flags (including groups)
     * @param admin Admin entity
     * @return Combined flags string
     */
    std::string ResolveFlags(const database::Admin& admin);

    /**
     * @brief Resolve admin's effective immunity (including groups)
     * @param admin Admin entity
     * @return Maximum immunity level
     */
    int ResolveImmunity(const database::Admin& admin);

    std::unordered_map<int64_t, database::Admin> m_admins;
    std::unordered_map<std::string, database::AdminGroup> m_groups;
    mutable std::mutex m_mutex;
};

} // namespace admin
