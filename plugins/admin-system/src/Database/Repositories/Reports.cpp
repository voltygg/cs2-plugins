#include "Database/Repositories/Reports.hpp"

#include "Database/Tables/Schema.hpp"

#include <utility>

namespace AdminSystem::Database
{

void ReportRepository::CreateAsync(const Report& report, std::function<void(bool)> onDone)
{
    _db.RunAsync(
        "create_player_report",
        [report](auto& conn) {
            const Tables::PlayerReports t;
            conn(sqlpp::insert_into(t).set(t.reporterSteamId = report.ReporterSteamId,
                                           t.reporterName = report.ReporterName, t.reporterIp = report.ReporterIp,
                                           t.targetSteamId = report.TargetSteamId, t.targetName = report.TargetName,
                                           t.targetIp = report.TargetIp, t.reasonCode = report.ReasonCode,
                                           t.reason = report.Reason, t.serverTag = report.ServerTag,
                                           t.mapName = report.MapName, t.createdAt = report.CreatedAt));
        },
        [onDone = std::move(onDone)](VoltMod::Status result) {
            if (onDone)
                onDone(result.has_value());
        });
}

}  // namespace AdminSystem::Database
