#pragma once

#include "../../vendor/cs2-kit/src/Core/Singleton.hpp"
#include "Player.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace AdminSystem::Players
{

/**
 * Manages all connected players, indexed by slot and SteamID.
 * Main-thread-only (no mutex) — all access happens from game thread callbacks.
 */
class PlayerManager : public CS2Kit::Core::Singleton<PlayerManager>
{
public:
    explicit PlayerManager(Token) {}

    Player* AddPlayer(int slot, int64_t steamId, const std::string& name, const std::string& ipAddress);
    void RemovePlayer(int slot);
    void Clear();
    Player* GetPlayerBySlot(int slot);
    Player* GetPlayerBySteamId(int64_t steamId);
    std::vector<Player*> FindPlayersByName(const std::string& name);
    std::vector<Player*> GetAllPlayers();
    size_t GetPlayerCount() const;

private:
    std::unordered_map<int, std::unique_ptr<Player>> _playersBySlot;
    std::unordered_map<int64_t, Player*> _playersBySteamId;
};

}  // namespace AdminSystem::Players
