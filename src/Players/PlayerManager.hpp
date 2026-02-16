#pragma once

#include "Player.hpp"
#include "../Core/Singleton.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace AdminSystem::Players {

class PlayerManager : public Core::Singleton<PlayerManager> {
    friend class Core::Singleton<PlayerManager>;

public:
    Player* AddPlayer(int slot, int64_t steamId, const std::string& name, const std::string& ipAddress);
    void RemovePlayer(int slot);
    void Clear();
    Player* GetPlayerBySlot(int slot);
    Player* GetPlayerBySteamId(int64_t steamId);
    std::vector<Player*> FindPlayersByName(const std::string& name);
    std::vector<Player*> GetAllPlayers();
    size_t GetPlayerCount() const;

private:
    PlayerManager() = default;

    std::unordered_map<int, std::unique_ptr<Player>> _playersBySlot;
    std::unordered_map<int64_t, Player*> _playersBySteamId;
};

} // namespace AdminSystem::Players
