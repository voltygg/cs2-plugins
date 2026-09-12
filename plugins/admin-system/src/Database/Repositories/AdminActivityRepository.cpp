#include "AdminActivityRepository.hpp"

#include "../Mapping.hpp"

#include <VoltMod/Core/Time.hpp>
#include <string>
#include <utility>

namespace AdminSystem::Database
{

using VoltMod::Time;

void AdminActivityRepository::RecordAsync(int64_t adminSteamId, std::string_view adminName, std::string_view action,
                                     int64_t targetSteamId, std::string_view targetName, std::string_view detail,
                                     std::string_view serverTag)
{
    // The job outlives this call, so every view is copied rather than captured.
    _db.RunAsync("record_admin_activity", [adminSteamId, adminName = std::string(adminName), action = std::string(action),
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
    _db.RunAsync(
        "count_admin_activity",
        [adminSteamId, sinceEpoch](auto& conn) {
            const Tables::AdminActivity t;
            ActivityCounts counts;
            for (const auto& row : conn(sqlpp::select(t.action, sqlpp::count(t.id).as(total))
                                            .from(t)
                                            .where(t.adminSteamId == adminSteamId and t.createdAt >= sinceEpoch)
                                            .group_by(t.action)))
            {
                const auto count = static_cast<int>(row.total);
                const std::string_view action = row.action;
                if (action == "ban")
                    counts.Bans += count;
                else if (action == "kick")
                    counts.Kicks += count;
                else if (action == "voice_mute" || action == "text_mute")
                    counts.Mutes += count;
                else if (action == "warn")
                    counts.Warnings += count;
            }
            return counts;
        },
        std::move(onDone));
}

}  // namespace AdminSystem::Database
