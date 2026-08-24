#pragma once

#include <VoltMod/Core/TimeUtils.hpp>
#include <VoltMod/Database/Column.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>

namespace AdminSystem::Database
{

/** Database entity for a text-chat mute. Supports permanent and timed mutes. */
struct TextMute
{
    int64_t Id = 0;
    int64_t TargetSteamId = 0;
    std::string TargetName;
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
    bool IsExpired() const { return !IsPermanent() && VoltMod::Core::TimeUtils::IsExpired(ExpiresAt); }

    static constexpr const char* Table = "text_mutes";
    static constexpr const char* Key = "id";
    static constexpr auto Columns()
    {
        using VoltMod::Database::Column;
        return std::tuple{
            Column{"id", &TextMute::Id},
            Column{"target_steam_id", &TextMute::TargetSteamId},
            Column{"target_name", &TextMute::TargetName},
            Column{"admin_steam_id", &TextMute::AdminSteamId},
            Column{"admin_name", &TextMute::AdminName},
            Column{"reason", &TextMute::Reason},
            Column{"created_at", &TextMute::CreatedAt},
            Column{"expires_at", &TextMute::ExpiresAt},
            Column{"duration", &TextMute::Duration},
            Column{"is_active", &TextMute::IsActive},
            Column{"removed_at", &TextMute::RemovedAt},
            Column{"removed_by", &TextMute::RemovedBy},
            Column{"removed_reason", &TextMute::RemovedReason},
        };
    }
};

}  // namespace AdminSystem::Database
