#pragma once

#include "CheatCheckMode.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Sdk/MoveType.hpp>
#include <cstdint>
#include <string>

namespace AdminSystem::Admin::CheatCheck
{

/** Per-target pending cheat-check state. Indexed by the suspect's slot. */
struct PendingCheck
{
    bool Active = false;
    int AdminSlot = -1;
    int64_t AdminSteamId = 0;
    CheatCheckMode Mode = CheatCheckMode::FixedLink;
    int64_t DeadlineSec = 0;  // Unix timestamp (TimeUtils::Now) at which the check times out
    VoltMod::Subscription TickTimer;
    std::string ResolvedUrl;  // URL shown to the suspect (empty while awaiting / before playerProvided submit)
    bool AwaitingUrl = false;
    uint64_t RequestSeq = 0;                                  // staleness guard for async HTTP completions
    std::string RoomCode;                                     // raw playerUrlField value; "" => no presence polling
    bool SuspectJoined = false;                               // countdown is paused while the suspect is in the room
    int64_t PausedRemainingSec = 0;                           // seconds that were left when the countdown paused
    int64_t NextPollAtSec = 0;                                // TimeUtils::Now timestamp of the next presence poll
    bool PollInFlight = false;                                // suppress overlapping polls
    VoltMod::MoveType PriorMoveType = VoltMod::MoveType::Walk;  // restored on unfreeze
    int PriorTeam = 0;                                        // team before force-to-spectator; restored on unfreeze
};

}  // namespace AdminSystem::Admin::CheatCheck
