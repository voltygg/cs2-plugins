#pragma once

#include <cstdint>
#include <string>

namespace AdminSystem::Database {

struct Ban {
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
    int64_t RemovedAt = 0;
    int64_t RemovedBy = 0;
    std::string RemovedReason;

    bool IsPermanent() const { return ExpiresAt == 0; }
    bool IsExpired() const;
};

} // namespace AdminSystem::Database
