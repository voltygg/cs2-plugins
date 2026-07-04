#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace AdminSystem::Database
{

/** Counts of rate-limited action types inside an abuse-detection window. */
struct ActivityCounts
{
    int Bans = 0;
    int Kicks = 0;
    int Mutes = 0;  // voice + text combined
    int Warnings = 0;
};

/**
 * Write side of the admin_activity audit trail (every admin action, including kicks which
 * have no punishment row) plus the network-wide counting query behind auto-freeze detection.
 * Both run on the database worker; the audit insert is fire-and-forget.
 */
class AdminActivityRepository
{
public:
    void Record(int64_t adminSteamId, const std::string& adminName, const std::string& action, int64_t targetSteamId,
                const std::string& targetName, const std::string& detail, const std::string& serverTag);

    /** Per-type action counts for one admin since @p sinceEpoch, across all servers; delivered
     *  on the game thread (FIFO after any audit insert enqueued before it). */
    void CountSinceAsync(int64_t adminSteamId, int64_t sinceEpoch, std::function<void(ActivityCounts)> onDone);
};

}  // namespace AdminSystem::Database
