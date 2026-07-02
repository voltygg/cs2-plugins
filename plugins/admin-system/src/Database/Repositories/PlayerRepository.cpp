#include "PlayerRepository.hpp"

#include "../../Core/Managers.hpp"

#include <CS2Kit/Database/DbResult.hpp>
#include <CS2Kit/Database/PostgresDatabase.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>

namespace AdminSystem::Database
{

using CS2Kit::Database::TryOr;
using CS2Kit::Utils::TimeUtils;

bool PlayerRepository::RecordConnect(int64_t steamId, const std::string& name, const std::string& ipAddress)
{
    // Bots connect with xuid 0 and never get a players row.
    if (steamId <= 0)
        return false;

    return TryOr(false, "PlayerRepository::RecordConnect", [&] {
        App().Db.ExecutePrepared(
            "player_record_connect",
            "INSERT INTO players (steam_id, name, ip_address, first_seen, last_seen, total_connections) "
            "VALUES ($1, $2, $3, $4, $4, 1) "
            "ON CONFLICT (steam_id) DO UPDATE SET "
            "name = EXCLUDED.name, ip_address = EXCLUDED.ip_address, last_seen = EXCLUDED.last_seen, "
            "total_connections = players.total_connections + 1",
            steamId, name, ipAddress, TimeUtils::Now());
        return true;
    });
}

bool PlayerRepository::RecordDisconnect(int64_t steamId, const std::string& name, int64_t sessionSeconds)
{
    if (steamId <= 0)
        return false;

    return TryOr(false, "PlayerRepository::RecordDisconnect", [&] {
        App().Db.ExecutePrepared("player_record_disconnect",
                                 "UPDATE players SET name = $2, last_seen = $3, total_playtime = total_playtime + $4 "
                                 "WHERE steam_id = $1",
                                 steamId, name, TimeUtils::Now(), sessionSeconds > 0 ? sessionSeconds : 0);
        return true;
    });
}

}  // namespace AdminSystem::Database
