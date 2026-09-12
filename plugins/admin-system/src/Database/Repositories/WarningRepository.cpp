#include "WarningRepository.hpp"

#include "../Mapping.hpp"

#include <VoltMod/Core/Time.hpp>
#include <VoltMod/Database/Api.hpp>
#include <utility>

namespace AdminSystem::Database
{

using VoltMod::Time;

void WarningRepository::CreateAsync(const Warning& warning)
{
    _db.RunAsync("create_warning", [warning](auto& conn) {
        const Tables::Warnings t;
        conn(InsertWarning(t, warning));
    });
}

void WarningRepository::CountActiveAsync(int64_t steamId, std::function<void(int)> onDone)
{
    _db.RunAsync(
        "count_active_warnings",
        [steamId, now = Time::Now()](auto& conn) {
            const Tables::Warnings t;
            int active = 0;
            for (const auto& row : conn(sqlpp::select(sqlpp::count(t.id).as(total))
                                            .from(t)
                                            .where(t.targetSteamId == steamId and t.isActive == true and
                                                   (t.expiresAt == 0 or t.expiresAt > now))))
                active = static_cast<int>(row.total);
            return active;
        },
        std::move(onDone));
}

void WarningRepository::ClearAsync(int64_t steamId)
{
    _db.RunAsync("clear_warnings", [steamId](auto& conn) {
        const Tables::Warnings t;
        conn(sqlpp::update(t).set(t.isActive = false).where(t.targetSteamId == steamId and t.isActive == true));
    });
}

}  // namespace AdminSystem::Database
