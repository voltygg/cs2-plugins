#include "../../Core/Config.hpp"
#include "../../Core/Managers.hpp"
#include "CheatCheckManager.hpp"
#include "CheatCheckRoomApi.hpp"
#include "CheatCheckView.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/App/Services.hpp>
#include <CS2Kit/Http/HttpClient.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/ChatColors.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <algorithm>
#include <format>

using CS2Kit::App::Engine;

namespace AdminSystem::Admin::CheatCheck
{

using CS2Kit::Utils::TimeUtils;
namespace Log = CS2Kit::Utils::Log;
namespace ChatColors = CS2Kit::Utils::ChatColors;

namespace
{
// A site blip can briefly report a joined suspect as absent; never resume with less than this,
// so they aren't kicked before the next poll can correct the picture.
constexpr int64_t MinResumeSec = 15;
}  // namespace

void CheatCheckManager::PollPresenceIfDue(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    if (pc.RoomCode.empty() || pc.PollInFlight || TimeUtils::Now() < pc.NextPollAtSec)
        return;

    auto* target = Engine().Players.GetPlayerBySlot(targetSlot);
    if (!target)
        return;  // disconnect cleanup tears the check down

    const auto& cfg = App().Config.GetCheatCheck().websiteAutoRoom;
    auto request = BuildPresenceRequest(cfg, pc.RoomCode, target->GetSteamID());
    if (!request)
    {
        // presenceUrl was removed by a config reload mid-check; back off a full interval
        // instead of re-attempting every tick.
        pc.NextPollAtSec = TimeUtils::Now() + cfg.pollIntervalSec;
        return;
    }

    pc.PollInFlight = true;
    const uint64_t seq = pc.RequestSeq;
    CS2Kit::Http::Get(Engine().Http, std::move(*request), [this, targetSlot, seq](const CS2Kit::HttpResult& result) {
        OnPresenceResponse(targetSlot, seq, result);
    });
}

void CheatCheckManager::OnPresenceResponse(int targetSlot, uint64_t seq, const CS2Kit::HttpResult& result)
{
    if (!ValidSlot(targetSlot))
        return;
    auto& pc = _checks[targetSlot];
    if (!pc.Active || pc.RequestSeq != seq)  // stale: cancelled, expired, re-called, or slot reused
        return;

    const auto& cfg = App().Config.GetCheatCheck().websiteAutoRoom;
    pc.PollInFlight = false;
    pc.NextPollAtSec = TimeUtils::Now() + cfg.pollIntervalSec;

    const auto present = ParsePresence(cfg, result);
    if (!present)
    {
        // Fail-open: an unreachable/failing presence API keeps the current state (the countdown
        // runs as before a join; a paused check stays paused until the admin resolves it).
        Log::Warn("Cheat-check presence poll failed for room {}: {}", pc.RoomCode,
                  result.Ok ? std::format("status {}", result.StatusCode) : result.Error);
        return;
    }

    if (*present == pc.SuspectJoined)
        return;

    auto* target = Engine().Players.GetPlayerBySlot(targetSlot);
    const std::string targetName = target ? target->GetName() : std::string();

    if (*present)
    {
        pc.SuspectJoined = true;
        pc.PausedRemainingSec = std::max<int64_t>(pc.DeadlineSec - TimeUtils::Now(), 0);
        ReplyToAdmin(pc, [&targetName, adminSlot = pc.AdminSlot] {
            return std::format(
                "{}{}", ChatColors::Green,
                Engine().Utils.Translations.Get("cheatCheck.suspectJoined", adminSlot, {{"name", targetName}}));
        });
    }
    else
    {
        pc.SuspectJoined = false;
        pc.DeadlineSec = TimeUtils::Now() + std::max(pc.PausedRemainingSec, MinResumeSec);
        ReplyToAdmin(pc, [&targetName, adminSlot = pc.AdminSlot] {
            return std::format("{}{}", ChatColors::Red,
                               Engine().Utils.Translations.Get("cheatCheck.suspectLeft", adminSlot, {{"name", targetName}}));
        });
    }

    View::RenderPanel(targetSlot, pc);
}

}  // namespace AdminSystem::Admin::CheatCheck
