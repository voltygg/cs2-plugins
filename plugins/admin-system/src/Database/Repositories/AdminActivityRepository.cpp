#include "AdminActivityRepository.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Database/Api.hpp>
#include <utility>

namespace AdminSystem::Database
{

void AdminActivityRepository::Record(int64_t adminSteamId, const std::string& adminName, const std::string& action,
                                     int64_t targetSteamId, const std::string& targetName, const std::string& detail,
                                     const std::string& serverTag)
{
    _db.Exec("record_admin_activity",
             "INSERT INTO admin_activity (admin_steam_id, admin_name, action, target_steam_id, "
             "target_name, detail, server_tag) VALUES ($1, $2, $3, $4, $5, $6, $7)",
             pqxx::params{adminSteamId, adminName, action, targetSteamId, targetName, detail, serverTag});
}

void AdminActivityRepository::CountSinceAsync(int64_t adminSteamId, int64_t sinceEpoch,
                                              std::function<void(ActivityCounts)> onDone)
{
    _db.Query("count_admin_activity",
              "SELECT COUNT(*) FILTER (WHERE action = 'ban') AS bans, "
              "COUNT(*) FILTER (WHERE action = 'kick') AS kicks, "
              "COUNT(*) FILTER (WHERE action IN ('voice_mute', 'text_mute')) AS mutes, "
              "COUNT(*) FILTER (WHERE action = 'warn') AS warnings "
              "FROM admin_activity WHERE admin_steam_id = $1 AND created_at >= $2",
              pqxx::params{adminSteamId, sinceEpoch},
              [onDone = std::move(onDone)](CS2Kit::DbResult<pqxx::result> result) {
                  if (!result || result->empty() || !onDone)
                      return;
                  const auto& row = (*result)[0];
                  onDone(ActivityCounts{.Bans = row["bans"].as<int>(0),
                                        .Kicks = row["kicks"].as<int>(0),
                                        .Mutes = row["mutes"].as<int>(0),
                                        .Warnings = row["warnings"].as<int>(0)});
              });
}

}  // namespace AdminSystem::Database
