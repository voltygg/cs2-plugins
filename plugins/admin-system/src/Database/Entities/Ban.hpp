#pragma once

#include <VoltMod/Core/Time.hpp>
#include <VoltMod/Database/Column.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>

namespace AdminSystem::Database
{

/** Database entity for a player ban. Supports permanent and timed bans with removal tracking. */
struct Ban
{
    int64_t Id = 0;
    int64_t TargetSteamId = 0;
    std::string TargetName;
    std::string TargetIp;
    int64_t AdminSteamId = 0;
    std::string AdminName;
    std::string Reason;
    int64_t CreatedAt = 0;
    int64_t ExpiresAt = 0;
    int64_t Duration = 0;
    bool IsActive = true;
    std::optional<int64_t> RemovedAt;
    std::optional<int64_t> RemovedBy;
    std::optional<std::string> RemovedReason;

    bool IsPermanent() const { return ExpiresAt == 0; }
    bool IsExpired() const { return !IsPermanent() && VoltMod::Core::Time::IsExpired(ExpiresAt); }

    static constexpr const char* Table = "bans";
    static constexpr const char* Key = "id";
    static constexpr auto Columns()
    {
        using VoltMod::Database::Column;
        return std::tuple{
            Column{"id", &Ban::Id},
            Column{"target_steam_id", &Ban::TargetSteamId},
            Column{"target_name", &Ban::TargetName},
            Column{"target_ip", &Ban::TargetIp},
            Column{"admin_steam_id", &Ban::AdminSteamId},
            Column{"admin_name", &Ban::AdminName},
            Column{"reason", &Ban::Reason},
            Column{"created_at", &Ban::CreatedAt},
            Column{"expires_at", &Ban::ExpiresAt},
            Column{"duration", &Ban::Duration},
            Column{"is_active", &Ban::IsActive},
            Column{"removed_at", &Ban::RemovedAt},
            Column{"removed_by", &Ban::RemovedBy},
            Column{"removed_reason", &Ban::RemovedReason},
        };
    }
};

}  // namespace AdminSystem::Database
