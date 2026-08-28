#pragma once

#include <VoltMod/Database/Column.hpp>
#include <cstdint>
#include <string>
#include <string_view>
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

    static constexpr std::string_view Table = "warnings";
    static constexpr std::string_view Key = "id";
    static constexpr auto Columns()
    {
        using VoltMod::Column;
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
