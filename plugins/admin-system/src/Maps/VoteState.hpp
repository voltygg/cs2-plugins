#pragma once

#include "MapQuery.hpp"

namespace VoltMod
{
class Runtime;
}

namespace AdminSystem::Core
{
class ConfigManager;
}

namespace AdminSystem::Maps
{

class MapCycleState;

/**
 * The map vote an admin opens from the Map menu.
 *
 * App-owned. Players answer through the game's own yes/no panel, so there is no plugin-side
 * tally to keep: the engine collects the ballots and @ref VoltMod::Messaging::Vote reports
 * them. A passing vote queues the map for the end of the round rather than changing level
 * mid-round.
 */
class VoteState
{
public:
    /** All three must outlive this object; App declares them above it. */
    VoteState(VoltMod::Runtime& runtime, const Core::ConfigManager& config, MapCycleState& cycle);

    /** Put @p map to a yes/no vote. @return false when a vote is already running. */
    bool StartMapVote(const MapEntry& map, int callerSlot);

    /** Cancel a running map vote. @return false when none is running. */
    bool CancelVote();

private:
    VoltMod::Runtime& _rt;
    const Core::ConfigManager& _config;
    MapCycleState& _cycle;
};

}  // namespace AdminSystem::Maps
