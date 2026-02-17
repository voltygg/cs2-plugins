#pragma once

#include <cstdint>
#include <string>

namespace AdminSystem::Database
{

/** Database entity tracking a player's connection history and total playtime. */
struct PlayerRecord
{
    int64_t Id = 0;
    int64_t SteamId = 0;
    std::string Name;
    std::string IpAddress;
    int64_t FirstSeen = 0;
    int64_t LastSeen = 0;
    int32_t TotalConnections = 0;
    int64_t TotalPlaytime = 0;
};

}  // namespace AdminSystem::Database
