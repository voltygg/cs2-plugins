#include "ReportRepository.hpp"

#include "../Tables/PlayerTables.hpp"

#include <VoltMod/Database/Api.hpp>
#include <utility>

namespace AdminSystem::Database
{

void ReportRepository::CreateAsync(const Report& report, std::function<void(bool)> onDone)
{
    _db.Run(
        "create_player_report",
        [report](auto& conn) {
            const Tables::PlayerReports t;
            conn(Tables::InsertReport(t, report));
        },
        [onDone = std::move(onDone)](VoltMod::DbResult<void> result) {
            if (onDone)
                onDone(result.has_value());
        });
}

}  // namespace AdminSystem::Database
