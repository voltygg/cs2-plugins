#pragma once

#include <VoltMod/Database/Api.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

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

/** The admin_activity audit trail and the counts behind auto-freeze detection. Kicks live
 *  only here, having no punishment row. Inserts are fire-and-forget. */
class AdminActivityRepository
{
public:
    explicit AdminActivityRepository(VoltMod::Database& db) : _db(db) {}

    void RecordAsync(int64_t adminSteamId, std::string_view adminName, std::string_view action,
                     int64_t targetSteamId, std::string_view targetName, std::string_view detail,
                     std::string_view serverTag);

    /** Counts for one admin across every server. Jobs are FIFO, so an audit insert enqueued
     *  before this one is included. */
    void CountSinceAsync(int64_t adminSteamId, int64_t sinceEpoch, std::function<void(ActivityCounts)> onDone);

private:
    VoltMod::Database& _db;
};

}  // namespace AdminSystem::Database
