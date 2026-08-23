#include "ServerRepository.hpp"

#include <CS2Kit/Api.hpp>

namespace AdminSystem::Database
{

bool ServerRepository::Upsert(const std::string& tag, const std::string& name)
{
    auto result = _db.QueryBlocking("upsert_server",
                                    "INSERT INTO servers (tag, name) VALUES ($1, $2) "
                                    "ON CONFLICT (tag) DO UPDATE SET name = $2, "
                                    "last_seen = EXTRACT(EPOCH FROM NOW())::BIGINT",
                                    pqxx::params{tag, name});
    return result.has_value();
}

void ServerRepository::Heartbeat(const std::string& tag)
{
    _db.Exec("heartbeat_server", "UPDATE servers SET last_seen = EXTRACT(EPOCH FROM NOW())::BIGINT WHERE tag = $1",
             pqxx::params{tag});
}

std::unordered_map<int64_t, std::vector<std::string>> AdminServerGroupRepository::FindByServerTag(
    const std::string& serverTag)
{
    std::unordered_map<int64_t, std::vector<std::string>> grants;
    auto result = _db.QueryBlocking("find_server_groups",
                                    "SELECT admin_steam_id, group_name FROM admin_server_groups WHERE server_tag = $1",
                                    pqxx::params{serverTag});
    if (!result)
        return grants;

    for (const auto& row : *result)
        grants[row["admin_steam_id"].as<int64_t>()].push_back(row["group_name"].c_str());
    return grants;
}

}  // namespace AdminSystem::Database
