#pragma once

#include "MapQuery.hpp"

#include <VoltMod/Core/Subscription.hpp>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace VoltMod
{
class Runtime;
}

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Maps
{

/**
 * Player-driven map changes: `!rtv` and `!votemap`.
 *
 * App-owned. RTV accumulates one vote per SteamID and, once enough players agree, queues the
 * next map rather than changing level mid-round. `!votemap` puts one named map to the game's own
 * yes/no panel.
 */
class VoteState
{
public:
    explicit VoteState(App& app);

    /** Subscribe to the round and disconnect events RTV needs. Call once during load. */
    void Start();

    /** Result of one player calling `!rtv`. */
    enum class RtvResult
    {
        Disabled,
        TooEarly,  ///< still inside the post-map-start delay
        AlreadyVoted,
        Counted,
        Passed,
    };

    RtvResult CastRtv(int slot, int64_t steamId);

    /** Votes cast so far, and how many are needed. */
    std::size_t RtvVotes() const { return _rtvVoters.size(); }
    std::size_t RtvNeeded() const;

    /** Put @p map to a yes/no vote. @return false when a vote is already running. */
    bool StartMapVote(const MapEntry& map, int callerSlot);

    /** Cancel a running map vote. @return false when none is running. */
    bool CancelVote();

private:
    void Reset();
    std::size_t HumanCount() const;
    /** Queue @p map and tell everyone it won. */
    void QueueWinner(const MapEntry& map);

    App& _app;
    /** SteamIDs that have called !rtv this map, so a reconnect cannot vote twice. */
    std::unordered_set<int64_t> _rtvVoters;
    /** Map the tally belongs to; a change clears it. */
    std::string _currentMap;
    /** Monotonic seconds when the current map started, for the post-start RTV delay. */
    double _mapStartedAt = 0.0;
    bool _rtvPassed = false;
    std::vector<VoltMod::Subscription> _subs;
};

}  // namespace AdminSystem::Maps
