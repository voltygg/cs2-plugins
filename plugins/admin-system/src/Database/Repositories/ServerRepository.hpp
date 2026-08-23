#pragma once

#include <CS2Kit/Database/Api.hpp>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace AdminSystem::Database
{

/** Registration/heartbeat for this server's row in the servers table. */
class ServerRepository
{
public:
    explicit ServerRepository(CS2Kit::PostgresDatabase& db) : _db(db) {}

    /** Insert or refresh this server's registry row. Blocking - called once at boot. */
    bool Upsert(const std::string& tag, const std::string& name);

    /** Advance last_seen so operators can tell which registered servers are alive. Fire-and-forget. */
    void Heartbeat(const std::string& tag);

private:
    CS2Kit::PostgresDatabase& _db;
};

/** Read side of admin_server_groups: which extra groups each admin holds on one server. */
class AdminServerGroupRepository
{
public:
    explicit AdminServerGroupRepository(CS2Kit::PostgresDatabase& db) : _db(db) {}

    /** steamId -> group names granted on @p serverTag. Blocking - load-time only. */
    std::unordered_map<int64_t, std::vector<std::string>> FindByServerTag(const std::string& serverTag);

private:
    CS2Kit::PostgresDatabase& _db;
};

}  // namespace AdminSystem::Database
