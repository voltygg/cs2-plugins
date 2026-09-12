#include "ServerRepository.hpp"

#include "../Tables/Schema.hpp"

#include <VoltMod/Core/Time.hpp>
#include <utility>

namespace AdminSystem::Database
{

using VoltMod::Time;

bool ServerRepository::Upsert(const std::string& tag, const std::string& name)
{
    auto result = _db.Run("upsert_server", [tag, name, now = Time::Now()](auto& conn) {
        const Tables::Servers t;
        VoltMod::Upsert(conn, sqlpp::update(t).set(t.name = name, t.lastSeen = now).where(t.tag == tag),
                        sqlpp::insert_into(t).set(t.tag = tag, t.name = name, t.createdAt = now, t.lastSeen = now));
    });
    return result.has_value();
}

void ServerRepository::HeartbeatAsync(const std::string& tag)
{
    _db.RunAsync("heartbeat_server", [tag, now = Time::Now()](auto& conn) {
        const Tables::Servers t;
        conn(sqlpp::update(t).set(t.lastSeen = now).where(t.tag == tag));
    });
}

std::unordered_map<int64_t, std::vector<std::string>> AdminServerGroupRepository::FindByServerTag(
    const std::string& serverTag)
{
    return _db.RunOr("find_server_groups", [serverTag](auto& conn) {
        const Tables::AdminServerGroups t;
        std::unordered_map<int64_t, std::vector<std::string>> grants;
        for (const auto& row : conn(sqlpp::select(t.adminSteamId, t.groupName).from(t).where(t.serverTag == serverTag)))
            grants[row.adminSteamId].emplace_back(row.groupName);
        return grants;
    });
}

}  // namespace AdminSystem::Database
