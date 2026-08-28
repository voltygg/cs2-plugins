#include "AdminActivityRepository.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Database/Api.hpp>
#include <utility>

namespace AdminSystem::Database
{

void AdminActivityRepository::Record(int64_t adminSteamId, std::string_view adminName, std::string_view action,
                                     int64_t targetSteamId, std::string_view targetName, std::string_view detail,
                                     std::string_view serverTag)
{
    // The insert is enqueued for the database worker, so every text value is copied into the
    // parameter pack here rather than left pointing at the caller's storage.
    _db.Exec("record_admin_activity",
             "INSERT INTO admin_activity (admin_steam_id, admin_name, action, target_steam_id, "
             "target_name, detail, server_tag) VALUES ($1, $2, $3, $4, $5, $6, $7)",
             pqxx::params{adminSteamId, std::string(adminName), std::string(action), targetSteamId,
                          std::string(targetName), std::string(detail), std::string(serverTag)});
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
              [onDone = std::move(onDone)](VoltMod::DbResult<pqxx::result> result) {
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
