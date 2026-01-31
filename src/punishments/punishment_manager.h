#pragma once

#include "../database/entities/ban.h"
#include "../database/entities/mute.h"
#include "../database/entities/gag.h"
#include "../database/entities/warning.h"
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <mutex>

namespace punishments {

/**
 * @brief Punishment manager for enforcing and tracking punishments
 */
class PunishmentManager
{
public:
    /**
     * @brief Load active punishments from database
     */
    bool LoadActivePunishments();

    /**
     * @brief Check if player is banned
     * @param steam_id SteamID64
     * @return Ban if active
     */
    std::optional<database::Ban> GetActiveBan(int64_t steam_id);

    /**
     * @brief Check if player is muted
     */
    bool IsMuted(int64_t steam_id);

    /**
     * @brief Check if player is gagged
     */
    bool IsGagged(int64_t steam_id);

    /**
     * @brief Issue a ban
     */
    bool IssueBan(const database::Ban& ban);

    /**
     * @brief Issue a mute
     */
    bool IssueMute(const database::Mute& mute);

    /**
     * @brief Issue a gag
     */
    bool IssueGag(const database::Gag& gag);

    /**
     * @brief Issue a warning
     */
    bool IssueWarning(const database::Warning& warning);

    /**
     * @brief Remove a ban
     */
    bool RemoveBan(int64_t ban_id, int64_t removed_by, const std::string& reason);

    /**
     * @brief Remove a mute
     */
    bool RemoveMute(int64_t mute_id, int64_t removed_by, const std::string& reason);

    /**
     * @brief Remove a gag
     */
    bool RemoveGag(int64_t gag_id, int64_t removed_by, const std::string& reason);

    /**
     * @brief Expire old punishments (periodic check)
     */
    void ExpireOldPunishments();

    /**
     * @brief Get singleton instance
     */
    static PunishmentManager& Instance();

private:
    PunishmentManager() = default;
    ~PunishmentManager() = default;
    PunishmentManager(const PunishmentManager&) = delete;
    PunishmentManager& operator=(const PunishmentManager&) = delete;

    std::unordered_set<int64_t> m_mutedPlayers;   // Active mutes by SteamID
    std::unordered_set<int64_t> m_gaggedPlayers;  // Active gags by SteamID
    std::unordered_map<int64_t, database::Ban> m_activeBans;  // Active bans by SteamID
    mutable std::mutex m_mutex;
};

} // namespace punishments
