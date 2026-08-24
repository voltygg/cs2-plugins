#include "../../Core/Config.hpp"
#include "CheatCheckManager.hpp"
#include "CheatCheckRoomApi.hpp"
#include "CheatCheckView.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/ChatColors.hpp>
#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/TimeUtils.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Http/HttpClient.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <algorithm>
#include <format>

namespace AdminSystem::Admin::CheatCheck
{

using VoltMod::Core::TimeUtils;
namespace Log = VoltMod::Core::Log;
namespace ChatColors = VoltMod::Core::ChatColors;

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

    auto* target = _rt.Players.GetPlayerBySlot(targetSlot);
    if (!target)
        return;  // disconnect cleanup tears the check down

    const auto& cfg = _config.GetCheatCheck().websiteAutoRoom;
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
    VoltMod::Http::Get(_rt.Http, std::move(*request), [this, targetSlot, seq](const VoltMod::HttpResult& result) {
        OnPresenceResponse(targetSlot, seq, result);
    });
}

void CheatCheckManager::OnPresenceResponse(int targetSlot, uint64_t seq, const VoltMod::HttpResult& result)
{
    if (!ValidSlot(targetSlot))
        return;
    auto& pc = _checks[targetSlot];
    if (!pc.Active || pc.RequestSeq != seq)  // stale: cancelled, expired, re-called, or slot reused
        return;

    const auto& cfg = _config.GetCheatCheck().websiteAutoRoom;
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

    auto* target = _rt.Players.GetPlayerBySlot(targetSlot);
    const std::string targetName = target ? target->GetName() : std::string();

    if (*present)
    {
        pc.SuspectJoined = true;
        pc.PausedRemainingSec = std::max<int64_t>(pc.DeadlineSec - TimeUtils::Now(), 0);
        ReplyToAdmin(pc, [this, &targetName, adminSlot = pc.AdminSlot] {
            return std::format("{}{}", ChatColors::Green,
                               _rt.Translations.Get("cheatCheck.suspectJoined", adminSlot, {{"name", targetName}}));
        });
    }
    else
    {
        pc.SuspectJoined = false;
        pc.DeadlineSec = TimeUtils::Now() + std::max(pc.PausedRemainingSec, MinResumeSec);
        ReplyToAdmin(pc, [this, &targetName, adminSlot = pc.AdminSlot] {
            return std::format("{}{}", ChatColors::Red,
                               _rt.Translations.Get("cheatCheck.suspectLeft", adminSlot, {{"name", targetName}}));
        });
    }

    View::RenderPanel(_rt, _config, targetSlot, pc);
}

}  // namespace AdminSystem::Admin::CheatCheck
