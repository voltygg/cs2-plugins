#include "AdminActivityRepository.hpp"

#include "../../Core/Managers.hpp"

#include <CS2Kit/Database/DbResult.hpp>
#include <CS2Kit/Database/PostgresDatabase.hpp>

namespace AdminSystem::Database
{

using CS2Kit::Database::TryOr;

bool AdminActivityRepository::Record(int64_t adminSteamId, const std::string& adminName, const std::string& action,
                                     int64_t targetSteamId, const std::string& targetName, const std::string& detail,
                                     const std::string& serverTag)
{
    return TryOr(false, "AdminActivityRepository::Record", [&] {
        App().Db.ExecutePrepared("record_admin_activity",
                                 "INSERT INTO admin_activity (admin_steam_id, admin_name, action, target_steam_id, "
                                 "target_name, detail, server_tag) VALUES ($1, $2, $3, $4, $5, $6, $7)",
                                 adminSteamId, adminName, action, targetSteamId, targetName, detail, serverTag);
        return true;
    });
}

ActivityCounts AdminActivityRepository::CountSince(int64_t adminSteamId, int64_t sinceEpoch)
{
    return TryOr(ActivityCounts{}, "AdminActivityRepository::CountSince", [&] {
        auto result = App().Db.ExecutePrepared(
            "count_admin_activity",
            "SELECT COUNT(*) FILTER (WHERE action = 'ban') AS bans, "
            "COUNT(*) FILTER (WHERE action = 'kick') AS kicks, "
            "COUNT(*) FILTER (WHERE action IN ('voice_mute', 'text_mute')) AS mutes, "
            "COUNT(*) FILTER (WHERE action = 'warn') AS warnings "
            "FROM admin_activity WHERE admin_steam_id = $1 AND created_at >= $2",
            adminSteamId, sinceEpoch);

        const auto& row = result[0];
        return ActivityCounts{.Bans = row["bans"].as<int>(0),
                              .Kicks = row["kicks"].as<int>(0),
                              .Mutes = row["mutes"].as<int>(0),
                              .Warnings = row["warnings"].as<int>(0)};
    });
}

}  // namespace AdminSystem::Database
