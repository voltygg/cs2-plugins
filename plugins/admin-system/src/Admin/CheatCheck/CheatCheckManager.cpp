#include "CheatCheckManager.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Core/Config.hpp"
#include "../Actions/Descriptors.hpp"
#include "CheatCheckRoomApi.hpp"
#include "CheatCheckView.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/ChatColors.hpp>
#include <CS2Kit/Core/Log.hpp>
#include <CS2Kit/Core/Scheduler.hpp>
#include <CS2Kit/Core/TimeUtils.hpp>
#include <CS2Kit/Core/Translations.hpp>
#include <CS2Kit/Http/HttpClient.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Runtime.hpp>
#include <CS2Kit/Sdk/PawnOps.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Sdk/UserMessage.hpp>
#include <format>

namespace AdminSystem::Admin::CheatCheck
{

using AdminSystem::Core::ChatService;
using AdminSystem::Core::ConfigManager;
using CS2Kit::Core::Scheduler;
using CS2Kit::Core::TimeUtils;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Sdk::MessageSystem;
using CS2Kit::Sdk::MoveType;
using CS2Kit::Sdk::PlayerController;
namespace Log = CS2Kit::Core::Log;
namespace ChatColors = CS2Kit::Core::ChatColors;

namespace
{
bool IsValidLink(const std::string& link)
{
    if (link.rfind("https://", 0) != 0 && link.rfind("http://", 0) != 0)
        return false;
    // A real URL has no whitespace or HTML-significant characters; reject panel-injection attempts.
    return link.find_first_of(" \t\r\n<>\"'") == std::string::npos;
}
}  // namespace

bool CheatCheckManager::StartCheck(int adminSlot, int targetSlot)
{
    if (!ValidSlot(adminSlot) || !ValidSlot(targetSlot))
        return false;

    auto& plrMgr = _app.Runtime.Players;
    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!admin || !target)
        return false;

    PlayerController targetCtrl(targetSlot);
    const auto& cfg = _app.Config.GetCheatCheck();

    // Re-call: keep the original movetype/team (target is already frozen/spectated, so reading now is stale).
    // PriorTeam is 0 (sentinel) unless we actually move them, so the restore decision survives a config reload.
    const bool wasActive = _checks[targetSlot].Active;
    const MoveType priorMove = wasActive ? _checks[targetSlot].PriorMoveType : targetCtrl.GetMoveType();
    const int priorTeam = wasActive ? _checks[targetSlot].PriorTeam : (cfg.moveToSpectator ? targetCtrl.GetTeam() : 0);
    if (wasActive)
        ResetCheck(targetSlot);

    auto& pc = _checks[targetSlot];
    pc.Active = true;
    pc.AdminSlot = adminSlot;
    pc.AdminSteamId = admin->GetSteamID();
    pc.Mode = ParseMode(cfg.mode);
    pc.DeadlineSec = TimeUtils::Now() + cfg.timeoutSec;
    pc.ResolvedUrl.clear();
    pc.AwaitingUrl = false;
    pc.RequestSeq = _seq++;
    pc.PriorMoveType = priorMove;
    pc.PriorTeam = priorTeam;

    targetCtrl.SetMoveType(MoveType::None);
    if (cfg.moveToSpectator)
        targetCtrl.ChangeTeam(CS2Kit::Sdk::TeamSpectator);

    int interval = cfg.panelRefreshMs > 0 ? cfg.panelRefreshMs : 100;
    pc.TickTimer = _app.Runtime.Scheduler.Repeat(interval, [this, targetSlot] { Tick(targetSlot); });

    ResolveUrl(targetSlot);
    View::Render(_app, targetSlot, pc);

    _app.Chat.BroadcastAction("broadcast.cheatCheckCalled", admin->GetName(), target->GetName());
    return true;
}

void CheatCheckManager::ResolveUrl(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    const auto& cfg = _app.Config.GetCheatCheck();

    switch (pc.Mode)
    {
    case CheatCheckMode::FixedLink:
        pc.ResolvedUrl = cfg.fixedLink.url;
        break;
    case CheatCheckMode::WebsiteAutoRoom:
        pc.AwaitingUrl = true;
        RequestRoom(targetSlot);
        break;
    case CheatCheckMode::PlayerProvided:
        break;  // the suspect supplies the URL via !cc
    }
}

void CheatCheckManager::RequestRoom(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    auto* target = _app.Runtime.Players.GetPlayerBySlot(targetSlot);

    std::optional<RoomRequest> request;
    if (target)
    {
        auto* admin = _app.Runtime.Players.GetPlayerBySlot(pc.AdminSlot);
        request = BuildRoomRequest(_app.Config.GetCheatCheck().websiteAutoRoom, target->GetSteamID(), target->GetName(),
                                   pc.AdminSteamId, admin ? admin->GetName() : std::string_view{});
    }

    if (!request)
    {
        // No endpoint to call (or the target vanished): fall back synchronously. StartCheck renders the
        // panel/chat afterward, so we only set state here (OnRoomFailed would double-send the instructions).
        FallbackToFixed(pc);
        return;
    }

    const uint64_t seq = pc.RequestSeq;
    CS2Kit::Http::Post(
        _app.Runtime.Http, std::move(*request),
        [this, targetSlot, seq](const CS2Kit::HttpResult& result) { OnRoomResponse(targetSlot, seq, result); });
}

void CheatCheckManager::OnRoomResponse(int targetSlot, uint64_t seq, const CS2Kit::HttpResult& result)
{
    if (!ValidSlot(targetSlot))
        return;
    auto& pc = _checks[targetSlot];
    if (!pc.Active || pc.RequestSeq != seq)  // stale: cancelled, expired, re-called, or slot reused
        return;

    const auto& roomCfg = _app.Config.GetCheatCheck().websiteAutoRoom;
    if (auto urls = ParseRoomResponse(roomCfg, result))
    {
        pc.ResolvedUrl = std::move(urls->PlayerUrl);
        pc.AwaitingUrl = false;

        // An empty RoomCode keeps the per-tick poll gate on its fast path when polling is off.
        if (!roomCfg.presenceUrl.empty())
        {
            pc.RoomCode = std::move(urls->RoomCode);
            // First poll one interval out: the suspect can't have opened the link yet.
            pc.NextPollAtSec = TimeUtils::Now() + roomCfg.pollIntervalSec;
        }

        if (!urls->CheckerUrl.empty())
            RelayCheckerUrl(targetSlot, urls->CheckerUrl);
        View::Render(_app, targetSlot, pc);
        return;
    }

    if (!result.Ok)
        Log::Warn("Cheat-check room request failed: {}", result.Error);
    else
        Log::Warn("Cheat-check room response rejected (status {}): {}", result.StatusCode, result.Body.substr(0, 300));

    OnRoomFailed(targetSlot);
}

void CheatCheckManager::OnRoomFailed(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    FallbackToFixed(pc);

    ReplyToAdmin(pc, [this, adminSlot = pc.AdminSlot] {
        return std::format("{}{}", ChatColors::Red, _app.Runtime.Translations.Get("cheatCheck.apiFailed", adminSlot));
    });

    View::Render(_app, targetSlot, pc);
}

void CheatCheckManager::RelayCheckerUrl(int targetSlot, const std::string& checkerUrl)
{
    if (auto slot = ResolveAdminSlot(_checks[targetSlot]))
    {
        _app.Chat.ReplyLink(
            *slot,
            std::format("{}{}", ChatColors::Green, _app.Runtime.Translations.Get("cheatCheck.checkerUrl", *slot)),
            checkerUrl);
    }
}

void CheatCheckManager::Tick(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    if (!pc.Active)
        return;

    // While the suspect is in the check room the deadline is suspended; polling still runs so
    // leaving the room resumes the countdown.
    if (!pc.SuspectJoined && TimeUtils::Now() >= pc.DeadlineSec)
    {
        Expire(targetSlot);
        return;
    }

    PollPresenceIfDue(targetSlot);
    View::RenderPanel(_app, targetSlot, pc);
}

CheatCheckManager::SubmitResult CheatCheckManager::SubmitPlayerLink(int callerSlot, const std::string& link)
{
    if (!ValidSlot(callerSlot))
        return SubmitResult::NoActiveCheck;

    auto& pc = _checks[callerSlot];
    if (!pc.Active || pc.Mode != CheatCheckMode::PlayerProvided)
        return SubmitResult::NoActiveCheck;

    if (!IsValidLink(link))
        return SubmitResult::Invalid;

    pc.ResolvedUrl = link;

    auto* caller = _app.Runtime.Players.GetPlayerBySlot(callerSlot);
    std::string name = caller ? caller->GetName() : std::string();

    if (auto slot = ResolveAdminSlot(pc))
    {
        _app.Chat.ReplyLink(
            *slot,
            std::format("{}{} {}{}", ChatColors::Green, _app.Runtime.Translations.Get("cheatCheck.linkReceived", *slot),
                        ChatColors::Default, name),
            link);
    }

    View::RenderPanel(_app, callerSlot, pc);
    return SubmitResult::Relayed;
}

void CheatCheckManager::FallbackToFixed(PendingCheck& pc)
{
    pc.AwaitingUrl = false;
    const auto& cfg = _app.Config.GetCheatCheck();
    if (!cfg.fixedLink.url.empty())
        pc.ResolvedUrl = cfg.fixedLink.url;
}

std::optional<int> CheatCheckManager::ResolveAdminSlot(const PendingCheck& pc) const
{
    if (!_app.Runtime.Players.GetPlayerBySlotIfSteamId(pc.AdminSlot, pc.AdminSteamId))
        return std::nullopt;
    return pc.AdminSlot;
}

void CheatCheckManager::ReplyToAdmin(const PendingCheck& pc, const std::function<std::string()>& buildMessage)
{
    if (auto slot = ResolveAdminSlot(pc))
        _app.Chat.Reply(*slot, buildMessage());
}

void CheatCheckManager::ResetCheck(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    if (pc.TickTimer)
        _app.Runtime.Scheduler.Cancel(pc.TickTimer);
    _app.Runtime.Messages.ClearCenterHtml(targetSlot);
    pc = PendingCheck{};
}

bool CheatCheckManager::Cancel(int targetSlot)
{
    if (!ValidSlot(targetSlot) || !_checks[targetSlot].Active)
        return false;

    auto* target = _app.Runtime.Players.GetPlayerBySlot(targetSlot);
    std::string targetName = target ? target->GetName() : std::string();

    const MoveType restore = _checks[targetSlot].PriorMoveType;
    const int restoreTeam = _checks[targetSlot].PriorTeam;
    ResetCheck(targetSlot);
    Unfreeze(targetSlot, restore, restoreTeam);

    _app.Chat.BroadcastAction("broadcast.cheatCheckCleared", "", targetName);
    return true;
}

void CheatCheckManager::Expire(int targetSlot)
{
    const auto& cfg = _app.Config.GetCheatCheck();
    const bool kick = cfg.autoKick;

    auto* target = _app.Runtime.Players.GetPlayerBySlot(targetSlot);
    std::string targetName = target ? target->GetName() : std::string();

    const MoveType restore = _checks[targetSlot].PriorMoveType;
    const int restoreTeam = _checks[targetSlot].PriorTeam;
    ResetCheck(targetSlot);  // deactivate before the kick triggers disconnect cleanup

    if (kick)
        PlayerController(targetSlot).Kick(cfg.kickReason.c_str());
    else
        Unfreeze(targetSlot, restore, restoreTeam);

    _app.Chat.BroadcastAction("broadcast.cheatCheckTimedOut", "", targetName);
}

void CheatCheckManager::Unfreeze(int targetSlot, MoveType restoreMove, int restoreTeam)
{
    PlayerController pc(targetSlot);
    if (!pc.IsValid())
        return;
    // restoreTeam is a real playing team (T/CT) only if we actually pulled them to spectator at start.
    if (restoreTeam >= CS2Kit::Sdk::TeamT)
        pc.ChangeTeam(restoreTeam);
    pc.SetMoveType(restoreMove);
}

void CheatCheckManager::CancelAllForSlot(int slot)
{
    if (ValidSlot(slot) && _checks[slot].Active)
        ResetCheck(slot);  // silent: the player is gone, no unfreeze/broadcast
}

void CheatCheckManager::CancelAll()
{
    for (int slot = 0; slot < MaxSlots; ++slot)
    {
        if (!_checks[slot].Active)
            continue;
        Unfreeze(slot, _checks[slot].PriorMoveType, _checks[slot].PriorTeam);
        ResetCheck(slot);
    }
}

}  // namespace AdminSystem::Admin::CheatCheck
