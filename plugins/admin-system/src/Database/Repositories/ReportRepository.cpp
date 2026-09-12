#include "ReportRepository.hpp"

#include "../Mapping.hpp"

#include <VoltMod/Database/Api.hpp>
#include <utility>

namespace AdminSystem::Database
{

void ReportRepository::CreateAsync(const Report& report, std::function<void(bool)> onDone)
{
    _db.RunAsync(
        "create_player_report",
        [report](auto& conn) {
            const Tables::PlayerReports t;
            conn(InsertReport(t, report));
        },
        [onDone = std::move(onDone)](VoltMod::Status result) {
            if (onDone)
                onDone(result.has_value());
        });
}

}  // namespace AdminSystem::Database
