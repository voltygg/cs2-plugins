#pragma once

#include "player.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace player {

/**
 * @brief PlayerManager class for managing connected players
 */
class PlayerManager
{
public:
    /**
     * @brief Add a player
     * @param slot Player slot
     * @param steam_id SteamID64
     * @param name Player name
     * @param ip_address IP address
     * @return Pointer to created player
     */
    Player* AddPlayer(int slot, int64_t steam_id, const std::string& name, const std::string& ip_address);

    /**
     * @brief Remove a player by slot
     * @param slot Player slot
     */
    void RemovePlayer(int slot);

    /**
     * @brief Get player by slot
     * @param slot Player slot
     * @return Pointer to player or nullptr
     */
    Player* GetPlayerBySlot(int slot);

    /**
     * @brief Get player by SteamID
     * @param steam_id SteamID64
     * @return Pointer to player or nullptr
     */
    Player* GetPlayerBySteamID(int64_t steam_id);

    /**
     * @brief Find players by name (partial match, case-insensitive)
     * @param name Name to search for
     * @return Vector of matching players
     */
    std::vector<Player*> FindPlayersByName(const std::string& name);

    /**
     * @brief Get all players
     * @return Vector of all players
     */
    std::vector<Player*> GetAllPlayers();

    /**
     * @brief Get player count
     * @return Number of connected players
     */
    size_t GetPlayerCount() const;

    /**
     * @brief Clear all players (on map change, etc.)
     */
    void Clear();

    /**
     * @brief Get singleton instance
     * @return PlayerManager instance
     */
    static PlayerManager& Instance();

private:
    PlayerManager() = default;
    ~PlayerManager() = default;
    PlayerManager(const PlayerManager&) = delete;
    PlayerManager& operator=(const PlayerManager&) = delete;

    std::unordered_map<int, std::unique_ptr<Player>> m_playersBySlot;
    std::unordered_map<int64_t, Player*> m_playersBySteamID;
    mutable std::mutex m_mutex;
};

} // namespace player
