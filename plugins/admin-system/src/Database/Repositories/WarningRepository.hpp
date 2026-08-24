#pragma once

#include "../Entities/Warning.hpp"

#include <VoltMod/Database/Api.hpp>
#include <cstdint>
#include <functional>

namespace AdminSystem::Database
{

/** Repository for player warning records - the write side plus the escalation count. All
 *  methods run on the database worker; the count arrives via callback (jobs are FIFO, so a
 *  count enqueued after a create sees it). */
class WarningRepository
{
public:
    explicit WarningRepository(VoltMod::PostgresDatabase& db) : _db(db) {}

    void CreateAsync(const Warning& warning);
    void CountActiveAsync(int64_t steamId, std::function<void(int count)> onDone);
    void ClearAsync(int64_t steamId);

private:
    VoltMod::PostgresDatabase& _db;
};

}  // namespace AdminSystem::Database
