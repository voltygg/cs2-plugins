#include "WarningRepository.hpp"

#include "../../Core/App.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/TimeUtils.hpp>
#include <CS2Kit/Database/Api.hpp>
#include <utility>

namespace AdminSystem::Database
{

using namespace CS2Kit::Database;
using CS2Kit::Core::TimeUtils;

void WarningRepository::CreateAsync(const Warning& warning)
{
    _db.Exec("create_warning", InsertSql<Warning>(), InsertParams(warning));
}

void WarningRepository::CountActiveAsync(int64_t steamId, std::function<void(int)> onDone)
{
    _db.Query("count_active_warnings",
              "SELECT COUNT(*) AS total FROM warnings WHERE target_steam_id = $1 AND is_active = true "
              "AND (expires_at = 0 OR expires_at > $2)",
              pqxx::params{steamId, TimeUtils::Now()},
              [onDone = std::move(onDone)](CS2Kit::DbResult<pqxx::result> result) {
                  if (result && !result->empty() && onDone)
                      onDone((*result)[0]["total"].as<int>());
              });
}

void WarningRepository::ClearAsync(int64_t steamId)
{
    _db.Exec("clear_warnings", "UPDATE warnings SET is_active = false WHERE target_steam_id = $1 AND is_active = true",
             pqxx::params{steamId});
}

}  // namespace AdminSystem::Database
