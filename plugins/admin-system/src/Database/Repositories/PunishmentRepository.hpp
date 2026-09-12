#pragma once

#include "../Tables/PunishmentTables.hpp"

#include <VoltMod/Core/Time.hpp>
#include <VoltMod/Database/Api.hpp>
#include <cstdint>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace AdminSystem::Database
{

/** Repository for Ban, VoiceMute, or TextMute records sharing the same table shape. */
template <typename TEntity>
class PunishmentRepository
{
public:
    using T = typename TEntity::Table;

    explicit PunishmentRepository(VoltMod::Database& db) : _db(db) {}

    /** Blocking - load-time only. */
    std::vector<TEntity> FindAllActive()
    {
        auto result = _db.RunBlocking(JobName("find_all_active"), ActiveQuery(VoltMod::Time::Now()));
        return result ? std::move(*result) : std::vector<TEntity>{};
    }

    /** Async snapshot for cache refresh; @p onDone runs on the game thread. */
    void FindAllActiveAsync(std::function<void(std::vector<TEntity>)> onDone)
    {
        _db.Run(JobName("find_all_active"), ActiveQuery(VoltMod::Time::Now()),
                [onDone = std::move(onDone)](VoltMod::DbResult<std::vector<TEntity>> result) {
                    if (result && onDone)
                        onDone(std::move(*result));
                });
    }

    /** Async insert; @p onId receives the generated row ID on the game thread. */
    void CreateAsync(const TEntity& record, std::function<void(int64_t)> onId = {})
    {
        _db.Run(
            JobName("create"),
            [record](auto& conn) {
                const T t;
                return VoltMod::InsertReturningId(conn, Tables::InsertPunishment(t, record), TableName());
            },
            [onId = std::move(onId)](VoltMod::DbResult<int64_t> result) {
                if (result && onId)
                    onId(*result);
            });
    }

    void RemoveAsync(int64_t recordId, int64_t removedBy, const std::string& reason)
    {
        _db.Run(JobName("remove"), [recordId, removedBy, reason, now = VoltMod::Time::Now()](auto& conn) {
            const T t;
            conn(sqlpp::update(t)
                     .set(t.isActive = false, t.removedAt = now, t.removedBy = removedBy, t.removedReason = reason)
                     .where(t.id == recordId));
        });
    }

    void ExpireOldAsync()
    {
        _db.Run(JobName("expire_old"), [now = VoltMod::Time::Now()](auto& conn) {
            const T t;
            conn(sqlpp::update(t).set(t.isActive = false).where(t.isActive == true and t.expiresAt > 0 and
                                                                t.expiresAt <= now));
        });
    }

private:
    static constexpr std::string_view TableName() { return sqlpp::name_tag_of_t<T>::name; }

    /** Both the blocking and the async read run the same select. */
    static auto ActiveQuery(int64_t now)
    {
        return [now](auto& conn) {
            const T t;
            std::vector<TEntity> records;
            for (const auto& row : conn(sqlpp::select(sqlpp::all_of(t))
                                            .from(t)
                                            .where(t.isActive == true and (t.expiresAt == 0 or t.expiresAt > now))))
                records.push_back(Tables::PunishmentFromRow<TEntity>(row));
            return records;
        };
    }

    // Distinct log label per table, e.g. "voice_mutes_find_all_active".
    static std::string JobName(std::string_view op) { return std::format("{}_{}", TableName(), op); }

    VoltMod::Database& _db;
};

}  // namespace AdminSystem::Database
