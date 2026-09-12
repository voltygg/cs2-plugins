#pragma once

#include "../Tables/PlayerTables.hpp"

#include <cstdint>
#include <string>

namespace AdminSystem::Database
{

/** Database entity for a player-submitted report against another player. Only the columns the
 *  game server writes; the website-owned triage columns (status/handled_by/handled_at/resolution)
 *  keep their database defaults. */
struct Report
{
    using Table = Tables::PlayerReports;

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
};

}  // namespace AdminSystem::Database
