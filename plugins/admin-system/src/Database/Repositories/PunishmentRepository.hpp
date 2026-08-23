#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/TimeUtils.hpp>
#include <CS2Kit/Database/Api.hpp>
#include <cstdint>
#include <format>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace AdminSystem::Database
{

/**
 * Repository for punishment records, templated over the entity (Ban / VoiceMute / TextMute)
 * since all three tables share an identical schema - the table name and SQL come from the
 * entity's column table. Load-time reads block on the database worker; gameplay writes are
 * fire-and-forget or callback-based.
 */
template <typename TEntity>
class PunishmentRepository
{
public:
    explicit PunishmentRepository(CS2Kit::PostgresDatabase& db) : _db(db) {}

    /** Blocking - load-time only. */
    std::vector<TEntity> FindAllActive()
    {
        auto result = _db.QueryBlocking(Stmt("find_all_active"), CS2Kit::SelectSql<TEntity>(ActiveWhere),
                                        pqxx::params{CS2Kit::TimeUtils::Now()});
        return result ? CS2Kit::FromResult<TEntity>(*result) : std::vector<TEntity>{};
    }

    /** Async snapshot for the periodic cache refresh; @p onDone runs on the game thread. */
    void FindAllActiveAsync(std::function<void(std::vector<TEntity>)> onDone)
    {
        _db.Query(Stmt("find_all_active"), CS2Kit::SelectSql<TEntity>(ActiveWhere),
                  pqxx::params{CS2Kit::TimeUtils::Now()},
                  [onDone = std::move(onDone)](CS2Kit::DbResult<pqxx::result> result) {
                      if (result && onDone)
                          onDone(CS2Kit::FromResult<TEntity>(*result));
                  });
    }

    /** Async insert; @p onId receives the generated row id on the game thread. */
    void CreateAsync(const TEntity& record, std::function<void(int64_t)> onId = {})
    {
        _db.Query(Stmt("create"), CS2Kit::InsertSql<TEntity>(), CS2Kit::InsertParams(record),
                  [onId = std::move(onId)](CS2Kit::DbResult<pqxx::result> result) {
                      if (result && !result->empty() && onId)
                          onId((*result)[0][0].template as<int64_t>());
                  });
    }

    void RemoveAsync(int64_t recordId, int64_t removedBy, const std::string& reason)
    {
        _db.Exec(Stmt("remove"),
                 std::format("UPDATE {} SET is_active = false, removed_at = $2, removed_by = $3, "
                             "removed_reason = $4 WHERE id = $1",
                             TEntity::Table),
                 pqxx::params{recordId, CS2Kit::TimeUtils::Now(), removedBy, reason});
    }

    void ExpireOldAsync()
    {
        _db.Exec(Stmt("expire_old"),
                 std::format("UPDATE {} SET is_active = false WHERE is_active = true AND expires_at > 0 AND "
                             "expires_at <= $1",
                             TEntity::Table),
                 pqxx::params{CS2Kit::TimeUtils::Now()});
    }

private:
    static constexpr const char* ActiveWhere = "is_active = true AND (expires_at = 0 OR expires_at > $1)";

    // Distinct prepared-statement name per table, e.g. "voice_mutes_find_all_active".
    std::string Stmt(std::string_view op) const { return std::format("{}_{}", TEntity::Table, op); }

private:
    CS2Kit::PostgresDatabase& _db;
};

}  // namespace AdminSystem::Database
