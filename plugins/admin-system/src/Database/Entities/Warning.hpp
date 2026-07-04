#pragma once

#include <CS2Kit/Database/Column.hpp>
#include <cstdint>
#include <string>
#include <tuple>

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

    static constexpr const char* Table = "warnings";
    static constexpr const char* Key = "id";
    static constexpr auto Columns()
    {
        using CS2Kit::Database::Column;
        return std::tuple{
            Column{"id", &Warning::Id},
            Column{"target_steam_id", &Warning::TargetSteamId},
            Column{"target_name", &Warning::TargetName},
            Column{"admin_steam_id", &Warning::AdminSteamId},
            Column{"admin_name", &Warning::AdminName},
            Column{"reason", &Warning::Reason},
            Column{"created_at", &Warning::CreatedAt},
            Column{"is_active", &Warning::IsActive},
            Column{"expires_at", &Warning::ExpiresAt},
        };
    }
};

}  // namespace AdminSystem::Database
