#pragma once

#include <cstdint>
#include <string>

namespace AdminSystem::Database
{

/** Repository for the `players` table - connection history and accumulated playtime. */
class PlayerRepository
{
public:
    /**
     * Upsert on connect: first connect inserts the row; reconnects refresh name/ip/last_seen
     *  and bump total_connections. No-op for bots (steamId <= 0).
     */
    bool RecordConnect(int64_t steamId, const std::string& name, const std::string& ipAddress);

    /**
     * Fold a finished session into the row: refresh name/last_seen and add the session's
     * seconds to total_playtime. No-op for bots (steamId <= 0).
     */
    bool RecordDisconnect(int64_t steamId, const std::string& name, int64_t sessionSeconds);
};

}  // namespace AdminSystem::Database
