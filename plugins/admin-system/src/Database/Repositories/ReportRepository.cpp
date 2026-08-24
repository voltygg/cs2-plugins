#include "ReportRepository.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Database/Api.hpp>
#include <utility>

namespace AdminSystem::Database
{

using namespace VoltMod::Database;

void ReportRepository::CreateAsync(const Report& report, std::function<void(bool)> onDone)
{
    _db.Query("create_player_report", InsertSql<Report>(), InsertParams(report),
              [onDone = std::move(onDone)](VoltMod::DbResult<pqxx::result> result) {
                  if (onDone)
                      onDone(result.has_value() && !result->empty());
              });
}

}  // namespace AdminSystem::Database
