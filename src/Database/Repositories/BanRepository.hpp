#pragma once

#include "../Entities/Ban.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <vector>

namespace AdminSystem::Database
{

/** Repository for ban records -- lookup, creation, removal, expiration, and history queries. */
class BanRepository
{
public:
    std::optional<Ban> FindActiveBySteamId(int64_t steamId);
    std::optional<Ban> FindActiveByIp(const std::string& ip);
    std::vector<Ban> FindAllActive();
    bool Create(const Ban& ban);
    bool Update(const Ban& ban);
    bool Remove(int64_t banId, int64_t removedBy, const std::string& reason);
    int ExpireOldBans();
    std::vector<Ban> GetHistory(int64_t steamId);

private:
    Ban ParseRow(const pqxx::row& row);
};

}  // namespace AdminSystem::Database
