#pragma once

#include <cstdint>
#include <string>

namespace AdminSystem::Database
{

/** Database entity for an admin-issued warning against a player. */
struct Warning
{
    int64_t Id = 0;
    int64_t TargetSteamId = 0;
    std::string TargetName;
    int64_t AdminSteamId = 0;
    std::string AdminName;
    std::string Reason;
    int64_t CreatedAt = 0;
    bool IsActive = true;
    int64_t ExpiresAt = 0;
};

}  // namespace AdminSystem::Database
