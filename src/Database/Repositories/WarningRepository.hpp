#pragma once

#include "../Entities/Warning.hpp"

#include <pqxx/pqxx>
#include <vector>

namespace AdminSystem::Database
{

/** Repository for player warning records — used by escalation logic and audit history. */
class WarningRepository
{
public:
    bool Create(Warning& warning);
    int CountActive(int64_t steamId);
    int Clear(int64_t steamId);
    std::vector<Warning> GetHistory(int64_t steamId);

private:
    Warning ParseRow(const pqxx::row& row);
};

}  // namespace AdminSystem::Database
