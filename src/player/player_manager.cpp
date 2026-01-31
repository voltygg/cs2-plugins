#include "player_manager.h"

#include "../utils/string.h"

namespace player
{

Player* PlayerManager::AddPlayer(int slot, int64_t steam_id, const std::string& name, const std::string& ip_address)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Remove existing player in this slot if any
    m_playersBySlot.erase(slot);

    // Create new player
    auto player = std::make_unique<Player>(slot, steam_id, name, ip_address);
    Player* player_ptr = player.get();

    // Store in both maps
    m_playersBySlot[slot] = std::move(player);
    m_playersBySteamID[steam_id] = player_ptr;

    return player_ptr;
}

void PlayerManager::RemovePlayer(int slot)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playersBySlot.find(slot);
    if (it != m_playersBySlot.end())
    {
        // Remove from SteamID map
        m_playersBySteamID.erase(it->second->GetSteamID());

        // Remove from slot map
        m_playersBySlot.erase(it);
    }
}

Player* PlayerManager::GetPlayerBySlot(int slot)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playersBySlot.find(slot);
    if (it != m_playersBySlot.end())
    {
        return it->second.get();
    }

    return nullptr;
}

Player* PlayerManager::GetPlayerBySteamID(int64_t steam_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playersBySteamID.find(steam_id);
    if (it != m_playersBySteamID.end())
    {
        return it->second;
    }

    return nullptr;
}

std::vector<Player*> PlayerManager::FindPlayersByName(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<Player*> results;

    for (const auto& pair : m_playersBySlot)
    {
        if (utils::String::ContainsIgnoreCase(pair.second->GetName(), name))
        {
            results.push_back(pair.second.get());
        }
    }

    return results;
}

std::vector<Player*> PlayerManager::GetAllPlayers()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<Player*> players;
    players.reserve(m_playersBySlot.size());

    for (const auto& pair : m_playersBySlot)
    {
        players.push_back(pair.second.get());
    }

    return players;
}

size_t PlayerManager::GetPlayerCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_playersBySlot.size();
}

void PlayerManager::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playersBySlot.clear();
    m_playersBySteamID.clear();
}

PlayerManager& PlayerManager::Instance()
{
    static PlayerManager instance;
    return instance;
}

}  // namespace player
