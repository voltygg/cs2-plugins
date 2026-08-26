#pragma once

#include <VoltMod/Database/Column.hpp>
#include <cstdint>
#include <string>
#include <tuple>

namespace AdminSystem::Database
{

/** Database entity for a player-submitted report against another player. */
struct Report
{
    int64_t Id = 0;
    int64_t ReporterSteamId = 0;
    std::string ReporterName;
    std::string ReporterIp;
    int64_t TargetSteamId = 0;
    std::string TargetName;
    std::string TargetIp;
    std::string ReasonCode;
    std::string Reason;
    std::string ServerTag;
    std::string MapName;
    int64_t CreatedAt = 0;

    static constexpr const char* Table = "player_reports";
    static constexpr const char* Key = "id";
    /** InsertSql/InsertParams write every entry except the key, so the website-owned triage columns
     *  (status/handled_by/handled_at/resolution) are omitted and keep their database defaults. */
    static constexpr auto Columns()
    {
        using VoltMod::Column;
        return std::tuple{
            Column{"id", &Report::Id},
            Column{"reporter_steam_id", &Report::ReporterSteamId},
            Column{"reporter_name", &Report::ReporterName},
            Column{"reporter_ip", &Report::ReporterIp},
            Column{"target_steam_id", &Report::TargetSteamId},
            Column{"target_name", &Report::TargetName},
            Column{"target_ip", &Report::TargetIp},
            Column{"reason_code", &Report::ReasonCode},
            Column{"reason", &Report::Reason},
            Column{"server_tag", &Report::ServerTag},
            Column{"map_name", &Report::MapName},
            Column{"created_at", &Report::CreatedAt},
        };
    }
};

}  // namespace AdminSystem::Database
