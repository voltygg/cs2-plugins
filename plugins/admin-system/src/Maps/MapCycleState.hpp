#pragma once

#include "../Config/ConfigManager.hpp"
#include "MapQuery.hpp"

#include <VoltMod/Runtime.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace AdminSystem::Maps
{

/** Scoreboard pause between announcing a map change and taking the server away. */
inline constexpr int64_t MapChangeAnnounceMs = 5000;

/**
 * The configured map list, the pending next map, and the level change itself.
 *
 * App-owned. Holds the list rather than VoltMod::Map, which is deliberately
 * listless: which maps an operator offers is configuration, not engine state.
 */
class MapCycleState
{
public:
    MapCycleState(VoltMod::Runtime& runtime, const Config::ConfigManager& config);

    /** Cross-check the configured names against the engine and log the ones it cannot load.
     *  Called once at load so a typo shows up there instead of on the first `!map`. */
    void VerifyAgainstEngine();

    const std::vector<MapEntry>& Cycle() const;

    /** Queue @p map for the end of the current round. Replaces any previous pick. */
    void SetNext(const MapEntry& map);

    /** The queued map, or nullopt when none is set. */
    const std::optional<MapEntry>& Next() const { return _next; }

    /** Change to @p map after @p delayMs, giving players time to read the announcement. */
    void ChangeAfter(const MapEntry& map, int64_t delayMs = MapChangeAnnounceMs);

    /** Change to the queued map after @p delayMs, then clear it. No-op when nothing is queued. */
    void ChangeToNext(int64_t delayMs = MapChangeAnnounceMs);

private:
    VoltMod::Runtime& _rt;
    const Config::ConfigManager& _config;
    std::optional<MapEntry> _next;
    /** The pending timed change; a new ChangeAfter replaces it, and unload cancels it. */
    VoltMod::Subscription _pendingChange;
};

}  // namespace AdminSystem::Maps
