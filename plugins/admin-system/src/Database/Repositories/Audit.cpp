#include "Audit.hpp"

#include "../Tables/Schema.hpp"

#include <VoltMod/Core/Time.hpp>
#include <string>
#include <string_view>
#include <utility>

namespace AdminSystem::Database
{

using VoltMod::Time;

// Names the COUNT column so the row reads as row.total.
SQLPP_CREATE_NAME_TAG(total);

void AdminActivityRepository::RecordAsync(int64_t adminSteamId, std::string_view adminName, std::string_view action,
                                          int64_t targetSteamId, std::string_view targetName, std::string_view detail,
                                          std::string_view serverTag)
{
    // The job outlives this call, so every view is copied rather than captured.
    _db.RunAsync("record_admin_activity",
                 [adminSteamId, adminName = std::string(adminName), action = std::string(action), targetSteamId,
                  targetName = std::string(targetName), detail = std::string(detail),
                  serverTag = std::string(serverTag), now = Time::Now()](auto& conn) {
                     const Tables::AdminActivity t;
                     conn(sqlpp::insert_into(t).set(t.adminSteamId = adminSteamId, t.adminName = adminName,
                                                    t.action = action, t.targetSteamId = targetSteamId,
                                                    t.targetName = targetName, t.detail = detail,
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
                const auto action = Punishments::ParseAuditAction(std::string_view(row.action));
                if (!action)
                    continue;

                const auto count = static_cast<int>(row.total);
                switch (*action)
                {
                case PunishType::Ban:
                    counts.Bans += count;
                    break;
                case PunishType::Kick:
                    counts.Kicks += count;
                    break;
                case PunishType::VoiceMute:
                case PunishType::TextMute:
                    counts.Mutes += count;
                    break;
                case PunishType::Warn:
                    counts.Warnings += count;
                    break;
                }
            }
            return counts;
        },
        std::move(onDone));
}

void ReportRepository::CreateAsync(const Report& report, std::function<void(bool)> onDone)
{
    _db.RunAsync(
        "create_player_report",
        [report](auto& conn) {
            const Tables::PlayerReports t;
            conn(sqlpp::insert_into(t).set(
                t.reporterSteamId = report.ReporterSteamId, t.reporterName = report.ReporterName,
                t.reporterIp = report.ReporterIp, t.targetSteamId = report.TargetSteamId,
                t.targetName = report.TargetName, t.targetIp = report.TargetIp, t.reasonCode = report.ReasonCode,
                t.reason = report.Reason, t.serverTag = report.ServerTag, t.mapName = report.MapName,
                t.createdAt = report.CreatedAt));
        },
        [onDone = std::move(onDone)](VoltMod::Status result) {
            if (onDone)
                onDone(result.has_value());
        });
}

}  // namespace AdminSystem::Database
