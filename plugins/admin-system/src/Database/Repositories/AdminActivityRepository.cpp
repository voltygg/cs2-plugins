#include "AdminActivityRepository.hpp"

#include "../Tables/AdminTables.hpp"

#include <VoltMod/Core/Time.hpp>
#include <string>
#include <utility>

namespace AdminSystem::Database
{

using VoltMod::Time;

void AdminActivityRepository::Record(int64_t adminSteamId, std::string_view adminName, std::string_view action,
                                     int64_t targetSteamId, std::string_view targetName, std::string_view detail,
                                     std::string_view serverTag)
{
    // The insert runs on the database worker later, so every text value is copied here rather
    // than left pointing at the caller's storage.
    _db.Run("record_admin_activity", [adminSteamId, adminName = std::string(adminName), action = std::string(action),
                                      targetSteamId, targetName = std::string(targetName), detail = std::string(detail),
                                      serverTag = std::string(serverTag), now = Time::Now()](auto& conn) {
        const Tables::AdminActivity t;
        conn(sqlpp::insert_into(t).set(t.adminSteamId = adminSteamId, t.adminName = adminName, t.action = action,
                                       t.targetSteamId = targetSteamId, t.targetName = targetName, t.detail = detail,
                                       t.serverTag = serverTag, t.createdAt = now));
    });
}

void AdminActivityRepository::CountSinceAsync(int64_t adminSteamId, int64_t sinceEpoch,
                                              std::function<void(ActivityCounts)> onDone)
{
    _db.Run(
        "count_admin_activity",
        [adminSteamId, sinceEpoch](auto& conn) {
            const Tables::AdminActivity t;
            ActivityCounts counts;
            for (const auto& row : conn(sqlpp::select(t.action, sqlpp::count(t.id).as(sqlpp::alias::count_))
                                            .from(t)
                                            .where(t.adminSteamId == adminSteamId and t.createdAt >= sinceEpoch)
                                            .group_by(t.action)))
            {
                const auto total = static_cast<int>(row.count_);
                const std::string_view action = row.action;
                if (action == "ban")
                    counts.Bans += total;
                else if (action == "kick")
                    counts.Kicks += total;
                else if (action == "voice_mute" || action == "text_mute")
                    counts.Mutes += total;
                else if (action == "warn")
                    counts.Warnings += total;
            }
            return counts;
        },
        [onDone = std::move(onDone)](VoltMod::DbResult<ActivityCounts> result) {
            if (!result || !onDone)
                return;
            onDone(*result);
        });
}

}  // namespace AdminSystem::Database
