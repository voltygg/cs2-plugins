#include "ServerRepository.hpp"

#include "../Tables/AdminTables.hpp"

#include <VoltMod/Core/Time.hpp>
#include <utility>

namespace AdminSystem::Database
{

using VoltMod::Time;

bool ServerRepository::Upsert(const std::string& tag, const std::string& name)
{
    auto result = _db.RunBlocking("upsert_server", [tag, name, now = Time::Now()](auto& conn) {
        const Tables::Servers t;
        // Update-then-insert: portable across the three drivers, which disagree on upsert syntax.
        if (conn(sqlpp::update(t).set(t.name = name, t.lastSeen = now).where(t.tag == tag)).affected_rows == 0)
            conn(sqlpp::insert_into(t).set(t.tag = tag, t.name = name, t.createdAt = now, t.lastSeen = now));
    });
    return result.has_value();
}

void ServerRepository::Heartbeat(const std::string& tag)
{
    _db.Run("heartbeat_server", [tag, now = Time::Now()](auto& conn) {
        const Tables::Servers t;
        conn(sqlpp::update(t).set(t.lastSeen = now).where(t.tag == tag));
    });
}

std::unordered_map<int64_t, std::vector<std::string>> AdminServerGroupRepository::FindByServerTag(
    const std::string& serverTag)
{
    auto result = _db.RunBlocking("find_server_groups", [serverTag](auto& conn) {
        const Tables::AdminServerGroups t;
        std::unordered_map<int64_t, std::vector<std::string>> grants;
        for (const auto& row : conn(sqlpp::select(t.adminSteamId, t.groupName).from(t).where(t.serverTag == serverTag)))
            grants[row.adminSteamId].emplace_back(row.groupName);
        return grants;
    });

    if (!result)
        return {};
    return std::move(*result);
}

}  // namespace AdminSystem::Database
