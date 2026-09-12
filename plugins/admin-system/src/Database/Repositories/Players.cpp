#include "Players.hpp"

#include "../Tables/Schema.hpp"

#include <VoltMod/Core/Time.hpp>

namespace AdminSystem::Database
{

using VoltMod::Time;

void PlayerRepository::RecordConnectAsync(int64_t steamId, const std::string& name, const std::string& ipAddress)
{
    // Bots connect with xuid 0 and never get a players row.
    if (steamId <= 0)
        return;

    _db.RunAsync("player_record_connect", [steamId, name, ipAddress, now = Time::Now()](auto& conn) {
        const Tables::Players t;
        VoltMod::Upsert(conn,
                        sqlpp::update(t)
                            .set(t.name = name, t.ipAddress = ipAddress, t.lastSeen = now,
                                 t.totalConnections = t.totalConnections + 1)
                            .where(t.steamId == steamId),
                        sqlpp::insert_into(t).set(t.steamId = steamId, t.name = name, t.ipAddress = ipAddress,
                                                  t.firstSeen = now, t.lastSeen = now, t.totalConnections = 1));
    });
}

void PlayerRepository::RecordDisconnectAsync(int64_t steamId, const std::string& name, int64_t sessionSeconds)
{
    if (steamId <= 0)
        return;

    _db.RunAsync(
        "player_record_disconnect",
        [steamId, name, now = Time::Now(), seconds = sessionSeconds > 0 ? sessionSeconds : int64_t{0}](auto& conn) {
            const Tables::Players t;
            conn(sqlpp::update(t)
                     .set(t.name = name, t.lastSeen = now, t.totalPlaytime = t.totalPlaytime + seconds)
                     .where(t.steamId == steamId));
        });
}

bool ServerRepository::Upsert(const std::string& tag, const std::string& name)
{
    auto result = _db.Run("upsert_server", [tag, name, now = Time::Now()](auto& conn) {
        const Tables::Servers t;
        VoltMod::Upsert(conn, sqlpp::update(t).set(t.name = name, t.lastSeen = now).where(t.tag == tag),
                        sqlpp::insert_into(t).set(t.tag = tag, t.name = name, t.createdAt = now, t.lastSeen = now));
    });
    return result.has_value();
}

void ServerRepository::HeartbeatAsync(const std::string& tag)
{
    _db.RunAsync("heartbeat_server", [tag, now = Time::Now()](auto& conn) {
        const Tables::Servers t;
        conn(sqlpp::update(t).set(t.lastSeen = now).where(t.tag == tag));
    });
}

}  // namespace AdminSystem::Database
