#pragma once

#include "../Mapping.hpp"

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
    using TableType = typename TEntity::Table;

    explicit PunishmentRepository(VoltMod::Database& db) : _db(db) {}

    /** Blocking - load-time only. */
    std::vector<TEntity> FindAllActive()
    {
        return _db.RunOr(JobName("find_all_active"), ActiveQuery(VoltMod::Time::Now()));
    }

    /** Async snapshot for cache refresh; @p onDone runs on the game thread. */
    void FindAllActiveAsync(std::function<void(std::vector<TEntity>)> onDone)
    {
        _db.RunAsync(JobName("find_all_active"), ActiveQuery(VoltMod::Time::Now()), std::move(onDone));
    }

    /** Async insert; @p onId receives the generated row ID on the game thread. */
    void CreateAsync(const TEntity& record, std::function<void(int64_t)> onId = {})
    {
        _db.RunAsync(
            JobName("create"),
            [record](auto& conn) {
                const TableType t;
                return VoltMod::Insert(conn, InsertPunishment(t, record), TableName());
            },
            std::move(onId));
    }

    void RemoveAsync(int64_t recordId, int64_t removedBy, const std::string& reason)
    {
        _db.RunAsync(JobName("remove"), [recordId, removedBy, reason, now = VoltMod::Time::Now()](auto& conn) {
            const TableType t;
            conn(sqlpp::update(t)
                     .set(t.isActive = false, t.removedAt = now, t.removedBy = removedBy, t.removedReason = reason)
                     .where(t.id == recordId));
        });
    }

    void ExpireOldAsync()
    {
        _db.RunAsync(JobName("expire_old"), [now = VoltMod::Time::Now()](auto& conn) {
            const TableType t;
            conn(sqlpp::update(t).set(t.isActive = false).where(t.isActive == true and t.expiresAt > 0 and
                                                                t.expiresAt <= now));
        });
    }

private:
    static constexpr std::string_view TableName() { return sqlpp::name_tag_of_t<TableType>::name; }

    /** Both the blocking and the async read run the same select. */
    static auto ActiveQuery(int64_t now)
    {
        return [now](auto& conn) {
            const TableType t;
            std::vector<TEntity> records;
            for (const auto& row : conn(sqlpp::select(sqlpp::all_of(t))
                                            .from(t)
                                            .where(t.isActive == true and (t.expiresAt == 0 or t.expiresAt > now))))
                records.push_back(PunishmentFromRow<TEntity>(row));
            return records;
        };
    }

    // Distinct log label per table, e.g. "voice_mutes_find_all_active".
    static std::string JobName(std::string_view op) { return std::format("{}_{}", TableName(), op); }

    VoltMod::Database& _db;
};

}  // namespace AdminSystem::Database
