#include "BanRepository.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/TimeUtils.hpp>
#include <CS2Kit/Database/Api.hpp>
#include <utility>

namespace AdminSystem::Database
{

using namespace CS2Kit::Database;
using CS2Kit::Core::TimeUtils;

namespace
{
constexpr const char* ActiveWhere = "is_active = true AND (expires_at = 0 OR expires_at > $1)";
}

std::vector<Ban> BanRepository::FindAllActive()
{
    auto result =
        _db.QueryBlocking("find_all_active_bans", SelectSql<Ban>(ActiveWhere), pqxx::params{TimeUtils::Now()});
    return result ? FromResult<Ban>(*result) : std::vector<Ban>{};
}

void BanRepository::FindAllActiveAsync(std::function<void(std::vector<Ban>)> onDone)
{
    _db.Query("find_all_active_bans", SelectSql<Ban>(ActiveWhere), pqxx::params{TimeUtils::Now()},
              [onDone = std::move(onDone)](CS2Kit::DbResult<pqxx::result> result) {
                  if (result && onDone)
                      onDone(FromResult<Ban>(*result));
              });
}

void BanRepository::CreateAsync(const Ban& ban, std::function<void(int64_t)> onId)
{
    _db.Query("create_ban", InsertSql<Ban>(), InsertParams(ban),
              [onId = std::move(onId)](CS2Kit::DbResult<pqxx::result> result) {
                  if (result && !result->empty() && onId)
                      onId((*result)[0][0].as<int64_t>());
              });
}

void BanRepository::RemoveAsync(int64_t banId, int64_t removedBy, const std::string& reason)
{
    _db.Exec("remove_ban",
             "UPDATE bans SET is_active = false, removed_at = $2, removed_by = $3, removed_reason = $4 WHERE id = $1",
             pqxx::params{banId, TimeUtils::Now(), removedBy, reason});
}

void BanRepository::ExpireOldAsync()
{
    _db.Exec("expire_old_bans",
             "UPDATE bans SET is_active = false WHERE is_active = true AND expires_at > 0 AND expires_at <= $1",
             pqxx::params{TimeUtils::Now()});
}

}  // namespace AdminSystem::Database
