#pragma once

#include "CheatCheckMode.hpp"

#include <CS2Kit/Sdk/MoveType.hpp>
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
    uint64_t TickTimer = 0;
    std::string ResolvedUrl;  // URL shown to the suspect (empty while awaiting / before playerProvided submit)
    bool AwaitingUrl = false;
    uint64_t RequestSeq = 0;                                            // staleness guard for async HTTP completions
    CS2Kit::Sdk::MoveType PriorMoveType = CS2Kit::Sdk::MoveType::Walk;  // restored on unfreeze
};

}  // namespace AdminSystem::Admin::CheatCheck
