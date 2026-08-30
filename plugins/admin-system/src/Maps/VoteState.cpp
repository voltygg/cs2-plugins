#include "VoteState.hpp"

#include "../Config/ConfigManager.hpp"
#include "MapCycleState.hpp"
#include "VoteMath.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <string_view>

namespace AdminSystem::Maps
{

/** The engine only renders its own vote tokens; arbitrary text does not appear. */
static constexpr std::string_view VoteTitleToken = "#SFUI_vote_changelevel";

VoteState::VoteState(VoltMod::Runtime& runtime, const Config::ConfigManager& config, MapCycleState& cycle)
    : _rt(runtime), _config(config), _cycle(cycle)
{}

bool VoteState::StartMapVote(const MapEntry& map, int callerSlot)
{
    if (_rt.Hooks.Vote.InProgress())
        return false;

    const auto& cfg = _config.GetMaps().vote;

    return _rt.Hooks.Vote.StartVote(
        VoteTitleToken, map.Label(), static_cast<float>(cfg.durationSec), callerSlot,
        [ratio = cfg.successRatio](const VoltMod::VoteTally& tally) {
            // Judged on the ballots actually cast, not on everyone connected: abstaining is not
            // the same as voting no.
            return tally.Cast() > 0 &&
                   VotePassed(static_cast<std::size_t>(tally.Yes), static_cast<std::size_t>(tally.Cast()), ratio);
        },
        // Queued rather than applied now, so a vote that lands mid-round does not cut it short.
        [this, map](bool passed, VoltMod::VoteEndReason) {
            if (passed)
                _cycle.SetNext(map);
        });
}

bool VoteState::CancelVote()
{
    if (!_rt.Hooks.Vote.InProgress())
        return false;
    _rt.Hooks.Vote.EndVote(VoltMod::VoteEndReason::Cancelled);
    return true;
}

}  // namespace AdminSystem::Maps
