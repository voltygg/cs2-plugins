#pragma once

#include "Config/ConfigManager.hpp"
#include "Maps/MapCycleState.hpp"
#include "Maps/MapQuery.hpp"

#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Maps
{

/**
 * The map vote an admin opens from the Map menu.
 *
 * App-owned. Players answer through the game's own yes/no panel, so there is no plugin-side
 * tally to keep: the engine collects the ballots and @ref VoltMod::Vote reports
 * them. A passing vote queues the map for the end of the round rather than changing level
 * mid-round.
 */
class VoteState
{
public:
    /** All three must outlive this object; App declares them above it. */
    VoteState(VoltMod::Runtime& runtime, const Config::ConfigManager& config, MapCycleState& cycle);

    /** Put @p map to a yes/no vote. @return false when a vote is already running. */
    bool StartMapVote(const MapEntry& map, int callerSlot);

    /** Cancel a running map vote. @return false when none is running. */
    bool CancelVote();

    /** Whether a vote is on screen right now, so the menu can gray Cancel instead of hiding it. */
    bool IsRunning() const { return _rt.Hooks.Vote.InProgress(); }

private:
    VoltMod::Runtime& _rt;
    const Config::ConfigManager& _config;
    MapCycleState& _cycle;
};

}  // namespace AdminSystem::Maps
