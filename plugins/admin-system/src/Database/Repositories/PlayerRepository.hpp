#pragma once

#include <VoltMod/Database/Api.hpp>
#include <cstdint>
#include <string>

namespace AdminSystem::Database
{

/** Repository for the `players` table - connection history and accumulated playtime.
 *  Both writes are fire-and-forget on the database worker. */
class PlayerRepository
{
public:
    explicit PlayerRepository(VoltMod::PostgresDatabase& db) : _db(db) {}

    /**
     * Upsert on connect: first connect inserts the row; reconnects refresh name/ip/last_seen
     * and bump total_connections. No-op for bots (steamId <= 0).
     */
    void RecordConnect(int64_t steamId, const std::string& name, const std::string& ipAddress);

    /**
     * Fold a finished session into the row: refresh name/last_seen and add the session's
     * seconds to total_playtime. No-op for bots (steamId <= 0).
     */
    void RecordDisconnect(int64_t steamId, const std::string& name, int64_t sessionSeconds);

private:
    VoltMod::PostgresDatabase& _db;
};

}  // namespace AdminSystem::Database
