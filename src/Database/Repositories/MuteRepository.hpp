#pragma once

#include "../Entities/Mute.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <vector>

namespace AdminSystem::Database
{

/** Repository for voice-mute records — lookup, creation, removal, expiration, and history. */
class MuteRepository
{
public:
    std::optional<Mute> FindActiveBySteamId(int64_t steamId);
    std::vector<Mute> FindAllActive();
    bool Create(Mute& mute);
    bool Remove(int64_t muteId, int64_t removedBy, const std::string& reason);
    int ExpireOldMutes();
    std::vector<Mute> GetHistory(int64_t steamId);

private:
    Mute ParseRow(const pqxx::row& row);
};

}  // namespace AdminSystem::Database
