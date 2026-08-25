#include "VoteState.hpp"

#include "../Core/App.hpp"
#include "../Core/ChatService.hpp"
#include "../Core/Config.hpp"
#include "MapCycleState.hpp"
#include "RtvCore.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/TimeUtils.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <string>

namespace AdminSystem::Maps
{

using VoltMod::Core::TimeUtils;

namespace
{

/** How long the yes/no map vote stays open. */
constexpr float MapVoteSeconds = 20.0f;

/** The engine only renders its own vote tokens; arbitrary text does not appear. */
constexpr const char* VoteTitleToken = "#SFUI_vote_changelevel";

}  // namespace

VoteState::VoteState(App& app) : _app(app) {}

void VoteState::Start()
{
    namespace Events = VoltMod::Events;
    auto& events = _app.Runtime.Events;

    _app.Runtime.Vote.Start();
    _mapStartedAt = TimeUtils::MonotonicSeconds();

    _currentMap = _app.Runtime.CurrentMap;

    // A new map clears the tally: an RTV is about leaving the map you are on. There is no map
    // -change event, so this notices the change at the first round of the new map.
    _subs.push_back(events.Listen<Events::RoundPrestart>([this](const Events::RoundPrestart&) {
        if (_app.Runtime.CurrentMap == _currentMap)
            return;
        _currentMap = _app.Runtime.CurrentMap;
        Reset();
    }));

    _subs.push_back(_app.Runtime.Slots.Listen([this](int) {
        // A player who leaves should not keep holding a vote, or a near-empty server can sit
        // one short of a threshold nobody can reach any more.
        if (_rtvPassed || _rtvVoters.empty())
            return;
        std::unordered_set<int64_t> present;
        for (auto* player : _app.Runtime.Players.GetAllPlayers())
        {
            if (player && !player->IsBot())
                present.insert(player->GetSteamID());
        }
        std::erase_if(_rtvVoters, [&present](int64_t id) { return !present.contains(id); });
    }));
}

void VoteState::Reset()
{
    _rtvVoters.clear();
    _rtvPassed = false;
    _mapStartedAt = TimeUtils::MonotonicSeconds();
}

std::size_t VoteState::HumanCount() const
{
    std::size_t humans = 0;
    for (auto* player : _app.Runtime.Players.GetAllPlayers())
    {
        if (player && !player->IsBot())
            ++humans;
    }
    return humans;
}

std::size_t VoteState::RtvNeeded() const
{
    return RtvThreshold(HumanCount(), _app.Config.GetMaps().rtv.successRatio);
}

VoteState::RtvResult VoteState::CastRtv(int /*slot*/, int64_t steamId)
{
    const auto& cfg = _app.Config.GetMaps().rtv;
    if (!cfg.enabled)
        return RtvResult::Disabled;

    if (TimeUtils::MonotonicSeconds() - _mapStartedAt < cfg.voteDelaySec)
        return RtvResult::TooEarly;

    if (_rtvPassed)
        return RtvResult::Passed;

    if (!_rtvVoters.insert(steamId).second)
        return RtvResult::AlreadyVoted;

    if (!RtvPassed(_rtvVoters.size(), HumanCount(), cfg.successRatio))
        return RtvResult::Counted;

    _rtvPassed = true;

    // Pick the map after the current one in the configured order, so RTV does not need its own
    // nomination flow to be useful.
    const auto& cycle = _app.MapCycle.Cycle();
    if (cycle.empty())
        return RtvResult::Passed;

    std::size_t next = 0;
    for (std::size_t i = 0; i < cycle.size(); ++i)
    {
        if (cycle[i].Name == _app.Runtime.CurrentMap)
        {
            next = (i + 1) % cycle.size();
            break;
        }
    }
    QueueWinner(cycle[next]);
    return RtvResult::Passed;
}

void VoteState::QueueWinner(const MapEntry& map)
{
    _app.MapCycle.SetNext(map);
    _app.Chat.BroadcastKey("broadcast.nextMapSet", {{"map", map.Label()}});
}

bool VoteState::StartMapVote(const MapEntry& map, int callerSlot)
{
    if (_app.Runtime.Vote.InProgress())
        return false;

    const auto& cfg = _app.Config.GetMaps().rtv;

    return _app.Runtime.Vote.StartVote(
        VoteTitleToken, map.Label(), MapVoteSeconds, callerSlot,
        [ratio = cfg.successRatio](const VoltMod::VoteTally& tally) {
            // Judged on the ballots actually cast, not on everyone connected: abstaining is not
            // the same as voting no.
            return tally.Cast() > 0 &&
                   RtvPassed(static_cast<std::size_t>(tally.Yes), static_cast<std::size_t>(tally.Cast()), ratio);
        },
        [this, map](bool passed, VoltMod::VoteEndReason) {
            if (passed)
                QueueWinner(map);
        });
}

bool VoteState::CancelVote()
{
    if (!_app.Runtime.Vote.InProgress())
        return false;
    _app.Runtime.Vote.EndVote(VoltMod::VoteEndReason::Cancelled);
    return true;
}

}  // namespace AdminSystem::Maps
