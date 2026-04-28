#pragma once

#include "../Entities/Gag.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <vector>

namespace AdminSystem::Database
{

/** Repository for chat-gag records — lookup, creation, removal, expiration, and history. */
class GagRepository
{
public:
    std::optional<Gag> FindActiveBySteamId(int64_t steamId);
    std::vector<Gag> FindAllActive();
    bool Create(Gag& gag);
    bool Remove(int64_t gagId, int64_t removedBy, const std::string& reason);
    int ExpireOldGags();
    std::vector<Gag> GetHistory(int64_t steamId);

private:
    Gag ParseRow(const pqxx::row& row);
};

}  // namespace AdminSystem::Database
