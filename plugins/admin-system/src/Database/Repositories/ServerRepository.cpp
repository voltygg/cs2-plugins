#include "ServerRepository.hpp"

#include "../../Core/Managers.hpp"

#include <CS2Kit/Database/DbResult.hpp>
#include <CS2Kit/Database/PostgresDatabase.hpp>

namespace AdminSystem::Database
{

using CS2Kit::Database::TryOr;

// ServerRepository implementation

bool ServerRepository::Upsert(const std::string& tag, const std::string& name)
{
    return TryOr(false, "ServerRepository::Upsert", [&] {
        App().Db.ExecutePrepared("upsert_server",
                                 "INSERT INTO servers (tag, name) VALUES ($1, $2) "
                                 "ON CONFLICT (tag) DO UPDATE SET name = $2, "
                                 "last_seen = EXTRACT(EPOCH FROM NOW())::BIGINT",
                                 tag, name);
        return true;
    });
}

bool ServerRepository::Heartbeat(const std::string& tag)
{
    return TryOr(false, "ServerRepository::Heartbeat", [&] {
        App().Db.ExecutePrepared(
            "heartbeat_server", "UPDATE servers SET last_seen = EXTRACT(EPOCH FROM NOW())::BIGINT WHERE tag = $1", tag);
        return true;
    });
}

// AdminServerGroupRepository implementation

std::unordered_map<int64_t, std::vector<std::string>> AdminServerGroupRepository::FindByServerTag(
    const std::string& serverTag)
{
    using GrantMap = std::unordered_map<int64_t, std::vector<std::string>>;
    return TryOr(GrantMap{}, "AdminServerGroupRepository::FindByServerTag", [&] {
        GrantMap grants;
        auto result = App().Db.ExecutePrepared(
            "find_server_groups", "SELECT admin_steam_id, group_name FROM admin_server_groups WHERE server_tag = $1",
            serverTag);

        for (const auto& row : result)
        {
            grants[row["admin_steam_id"].as<int64_t>()].push_back(row["group_name"].c_str());
        }
        return grants;
    });
}

}  // namespace AdminSystem::Database
