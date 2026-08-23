#include "ReportRepository.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Database/Api.hpp>
#include <utility>

namespace AdminSystem::Database
{

using namespace CS2Kit::Database;

void ReportRepository::CreateAsync(const Report& report, std::function<void(bool)> onDone)
{
    _db.Query("create_player_report", InsertSql<Report>(), InsertParams(report),
              [onDone = std::move(onDone)](CS2Kit::DbResult<pqxx::result> result) {
                  if (onDone)
                      onDone(result.has_value() && !result->empty());
              });
}

}  // namespace AdminSystem::Database
