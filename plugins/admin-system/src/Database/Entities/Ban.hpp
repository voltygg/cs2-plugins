#pragma once

#include "../Tables/Schema.hpp"

#include <VoltMod/Core/Time.hpp>
#include <cstdint>
#include <optional>
#include <string>

namespace AdminSystem::Database
{

/** Database entity for a player ban. Supports permanent and timed bans with removal tracking. */
struct Ban
{
    using Table = Tables::Bans;

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
    bool IsExpired() const { return !IsPermanent() && VoltMod::Time::IsExpired(ExpiresAt); }
};

}  // namespace AdminSystem::Database
