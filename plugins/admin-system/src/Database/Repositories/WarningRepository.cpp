#include "WarningRepository.hpp"

#include "../Tables/PunishmentTables.hpp"

#include <VoltMod/Core/Time.hpp>
#include <VoltMod/Database/Api.hpp>
#include <utility>

namespace AdminSystem::Database
{

using VoltMod::Time;

void WarningRepository::CreateAsync(const Warning& warning)
{
    _db.Run("create_warning", [warning](auto& conn) {
        const Tables::Warnings t;
        conn(Tables::InsertWarning(t, warning));
    });
}

void WarningRepository::CountActiveAsync(int64_t steamId, std::function<void(int)> onDone)
{
    _db.Run(
        "count_active_warnings",
        [steamId, now = Time::Now()](auto& conn) {
            const Tables::Warnings t;
            int total = 0;
            for (const auto& row : conn(sqlpp::select(sqlpp::count(t.id).as(sqlpp::alias::a))
                                            .from(t)
                                            .where(t.targetSteamId == steamId and t.isActive == true and
                                                   (t.expiresAt == 0 or t.expiresAt > now))))
                total = static_cast<int>(row.a);
            return total;
        },
        [onDone = std::move(onDone)](VoltMod::DbResult<int> result) {
            if (result && onDone)
                onDone(*result);
        });
}

void WarningRepository::ClearAsync(int64_t steamId)
{
    _db.Run("clear_warnings", [steamId](auto& conn) {
        const Tables::Warnings t;
        conn(sqlpp::update(t).set(t.isActive = false).where(t.targetSteamId == steamId and t.isActive == true));
    });
}

}  // namespace AdminSystem::Database
