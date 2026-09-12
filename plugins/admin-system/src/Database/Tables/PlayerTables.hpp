#pragma once

#include <VoltMod/Database/Table.hpp>
#include <optional>

namespace AdminSystem::Database::Tables
{

struct Players_
{
    VOLTMOD_COLUMN(id, id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(steamId, steam_id, sqlpp::integral, std::false_type);
    VOLTMOD_COLUMN(name, name, sqlpp::text, std::false_type);
    VOLTMOD_COLUMN(ipAddress, ip_address, std::optional<sqlpp::text>, std::true_type);
    VOLTMOD_COLUMN(firstSeen, first_seen, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(lastSeen, last_seen, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(totalConnections, total_connections, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(totalPlaytime, total_playtime, sqlpp::integral, std::true_type);

    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(players, players);
    template <typename T>
    using _table_columns =
        sqlpp::table_columns<T, id, steamId, name, ipAddress, firstSeen, lastSeen, totalConnections, totalPlaytime>;
    using _required_insert_columns = sqlpp::detail::type_set<sqlpp::column_t<sqlpp::table_t<Players_>, steamId>,
                                                              sqlpp::column_t<sqlpp::table_t<Players_>, name>>;
};
using Players = sqlpp::table_t<Players_>;

struct PlayerReports_
{
    VOLTMOD_COLUMN(id, id, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(reporterSteamId, reporter_steam_id, sqlpp::integral, std::false_type);
    VOLTMOD_COLUMN(reporterName, reporter_name, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(reporterIp, reporter_ip, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(targetSteamId, target_steam_id, sqlpp::integral, std::false_type);
    VOLTMOD_COLUMN(targetName, target_name, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(targetIp, target_ip, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(reasonCode, reason_code, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(reason, reason, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(serverTag, server_tag, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(mapName, map_name, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(createdAt, created_at, sqlpp::integral, std::true_type);
    // Website-owned triage columns; the plugin never writes these.
    VOLTMOD_COLUMN(status, status, sqlpp::text, std::true_type);
    VOLTMOD_COLUMN(handledBy, handled_by, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(handledAt, handled_at, sqlpp::integral, std::true_type);
    VOLTMOD_COLUMN(resolution, resolution, sqlpp::text, std::true_type);

    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(player_reports, player_reports);
    template <typename T>
    using _table_columns =
        sqlpp::table_columns<T, id, reporterSteamId, reporterName, reporterIp, targetSteamId, targetName, targetIp,
                              reasonCode, reason, serverTag, mapName, createdAt, status, handledBy, handledAt,
                              resolution>;
    using _required_insert_columns =
        sqlpp::detail::type_set<sqlpp::column_t<sqlpp::table_t<PlayerReports_>, reporterSteamId>,
                                 sqlpp::column_t<sqlpp::table_t<PlayerReports_>, targetSteamId>>;
};
using PlayerReports = sqlpp::table_t<PlayerReports_>;

template <class Entity>
auto InsertReport(const PlayerReports& t, const Entity& e)
{
    return sqlpp::insert_into(t).set(t.reporterSteamId = e.ReporterSteamId, t.reporterName = e.ReporterName,
                                      t.reporterIp = e.ReporterIp, t.targetSteamId = e.TargetSteamId,
                                      t.targetName = e.TargetName, t.targetIp = e.TargetIp,
                                      t.reasonCode = e.ReasonCode, t.reason = e.Reason, t.serverTag = e.ServerTag,
                                      t.mapName = e.MapName, t.createdAt = e.CreatedAt);
}

}  // namespace AdminSystem::Database::Tables
