#pragma once

#include <VoltMod/Database/Api.hpp>
#include <cstdint>
#include <string>

namespace AdminSystem::Database
{

/** Connection history and accumulated playtime. Both writes are fire-and-forget. */
class PlayerRepository
{
public:
    explicit PlayerRepository(VoltMod::Database& db) : _db(db) {}

    /** Inserts on first connect, refreshes and counts the visit after that. Bots have no row. */
    void RecordConnectAsync(int64_t steamId, const std::string& name, const std::string& ipAddress);

    /** Adds the finished session to total_playtime. Bots have no row. */
    void RecordDisconnectAsync(int64_t steamId, const std::string& name, int64_t sessionSeconds);

private:
    VoltMod::Database& _db;
};

/** Registration and heartbeat for this server's row in the servers table. */
class ServerRepository
{
public:
    explicit ServerRepository(VoltMod::Database& db) : _db(db) {}

    /** Insert or refresh this server's registry row. Blocking - called once at boot. */
    bool Upsert(const std::string& tag, const std::string& name);

    /** Advance last_seen so operators can tell which registered servers are alive. Fire-and-forget. */
    void HeartbeatAsync(const std::string& tag);

private:
    VoltMod::Database& _db;
};

}  // namespace AdminSystem::Database
