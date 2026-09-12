#include "PlayerRepository.hpp"

#include "../Tables/PlayerTables.hpp"

#include <VoltMod/Core/Time.hpp>
#include <VoltMod/Database/Api.hpp>

namespace AdminSystem::Database
{

using VoltMod::Time;

void PlayerRepository::RecordConnect(int64_t steamId, const std::string& name, const std::string& ipAddress)
{
    // Bots connect with xuid 0 and never get a players row.
    if (steamId <= 0)
        return;

    _db.Run("player_record_connect", [steamId, name, ipAddress, now = Time::Now()](auto& conn) {
        const Tables::Players t;
        const auto updated = conn(sqlpp::update(t)
                                      .set(t.name = name, t.ipAddress = ipAddress, t.lastSeen = now,
                                           t.totalConnections = t.totalConnections + 1)
                                      .where(t.steamId == steamId));
        if (updated.affected_rows == 0)
        {
            conn(sqlpp::insert_into(t).set(t.steamId = steamId, t.name = name, t.ipAddress = ipAddress,
                                           t.firstSeen = now, t.lastSeen = now, t.totalConnections = 1));
        }
    });
}

void PlayerRepository::RecordDisconnect(int64_t steamId, const std::string& name, int64_t sessionSeconds)
{
    if (steamId <= 0)
        return;

    _db.Run("player_record_disconnect",
            [steamId, name, now = Time::Now(), seconds = sessionSeconds > 0 ? sessionSeconds : int64_t{0}](auto& conn) {
                const Tables::Players t;
                conn(sqlpp::update(t)
                         .set(t.name = name, t.lastSeen = now, t.totalPlaytime = t.totalPlaytime + seconds)
                         .where(t.steamId == steamId));
            });
}

}  // namespace AdminSystem::Database
