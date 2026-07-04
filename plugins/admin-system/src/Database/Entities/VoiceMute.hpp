#pragma once

#include <CS2Kit/Database/Column.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>

namespace AdminSystem::Database
{

/** Database entity for a voice-chat mute. Supports permanent and timed mutes. */
struct VoiceMute
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
    bool IsExpired() const;

    static constexpr const char* Table = "voice_mutes";
    static constexpr const char* Key = "id";
    static constexpr auto Columns()
    {
        using CS2Kit::Database::Column;
        return std::tuple{
            Column{"id", &VoiceMute::Id},
            Column{"target_steam_id", &VoiceMute::TargetSteamId},
            Column{"target_name", &VoiceMute::TargetName},
            Column{"admin_steam_id", &VoiceMute::AdminSteamId},
            Column{"admin_name", &VoiceMute::AdminName},
            Column{"reason", &VoiceMute::Reason},
            Column{"created_at", &VoiceMute::CreatedAt},
            Column{"expires_at", &VoiceMute::ExpiresAt},
            Column{"duration", &VoiceMute::Duration},
            Column{"is_active", &VoiceMute::IsActive},
            Column{"removed_at", &VoiceMute::RemovedAt},
            Column{"removed_by", &VoiceMute::RemovedBy},
            Column{"removed_reason", &VoiceMute::RemovedReason},
        };
    }
};

}  // namespace AdminSystem::Database
