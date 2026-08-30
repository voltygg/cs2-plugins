#include "MapCycleState.hpp"

#include "../Config/ConfigManager.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Maps
{

namespace Log = VoltMod::Log;

MapCycleState::MapCycleState(VoltMod::Runtime& runtime, const Config::ConfigManager& config)
    : _rt(runtime), _config(config)
{}

void MapCycleState::VerifyAgainstEngine()
{
    for (const auto& map : Cycle())
    {
        // A workshop map is addressed by id and is not mounted yet, so the engine cannot
        // answer for it here; only plain names are checkable.
        if (map.WorkshopId != 0)
            continue;
        if (!_rt.Map.IsValid(map.Name))
            Log::Warn("maps.cycle: '{}' is not a map this server can load.", map.Name);
    }
}

const std::vector<MapEntry>& MapCycleState::Cycle() const
{
    return _config.GetMapCycle();
}

void MapCycleState::SetNext(const MapEntry& map)
{
    _next = map;
}

void MapCycleState::ChangeAfter(const MapEntry& map, int64_t delayMs)
{
    // Captures the engine service rather than `this`: the scheduler belongs to the runtime and
    // outlives this App-owned object, so a pending change must not reach back into plugin state.
    _pendingChange = _rt.Scheduler.Delay(delayMs, [&maps = _rt.Map, map] {
        if (map.WorkshopId != 0)
            maps.ChangeToWorkshop(map.WorkshopId);
        else
            maps.ChangeLevel(map.Name);
    });
}

void MapCycleState::ChangeToNext(int64_t delayMs)
{
    if (!_next)
        return;

    // Copy out and clear first: the delay outlives this call, and a second round end must not
    // queue the same change twice.
    MapEntry map = *_next;
    _next.reset();
    ChangeAfter(map, delayMs);
}

}  // namespace AdminSystem::Maps
