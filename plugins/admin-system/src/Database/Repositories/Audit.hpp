#pragma once

#include "../Entities.hpp"

#include <VoltMod/Database/Api.hpp>
#include <cstdint>
#include <functional>
#include <string_view>

namespace AdminSystem::Database
{

/** The admin_activity audit trail and the counts behind auto-freeze detection. Kicks live only
 *  here, having no punishment row. Inserts are fire-and-forget. */
class AdminActivityRepository
{
public:
    explicit AdminActivityRepository(VoltMod::Database& db) : _db(db) {}

    void RecordAsync(int64_t adminSteamId, std::string_view adminName, std::string_view action, int64_t targetSteamId,
                     std::string_view targetName, std::string_view detail, std::string_view serverTag);

    /** Counts for one admin across every server. Jobs are FIFO, so an audit insert enqueued
     *  before this one is included. */
    void CountSinceAsync(int64_t adminSteamId, int64_t sinceEpoch, std::function<void(ActivityCounts)> onDone);

private:
    VoltMod::Database& _db;
};

/** Player reports - insert only; the upstream website owns every read and the triage columns. */
class ReportRepository
{
public:
    explicit ReportRepository(VoltMod::Database& db) : _db(db) {}

    /** @p onDone gets the outcome on the game thread: the reporter is told whether it landed. */
    void CreateAsync(const Report& report, std::function<void(bool ok)> onDone = {});

private:
    VoltMod::Database& _db;
};

}  // namespace AdminSystem::Database
