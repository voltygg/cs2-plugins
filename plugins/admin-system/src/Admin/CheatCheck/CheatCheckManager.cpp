#include "CheatCheckManager.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Config.hpp"
#include "../Actions/Descriptors.hpp"
#include "CheatCheckRoomApi.hpp"
#include "CheatCheckView.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Log.hpp>
#include <VoltMod/Core/Scheduler.hpp>
#include <VoltMod/Core/Time.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Entities/PawnOps.hpp>
#include <VoltMod/Http/HttpClient.hpp>
#include <VoltMod/Messaging/ChatColors.hpp>
#include <VoltMod/Messaging/Messages.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>

namespace AdminSystem::Admin::CheatCheck
{

using VoltMod::Controller;
using VoltMod::MoveType;
using VoltMod::Pawn;
using VoltMod::Time;
namespace Log = VoltMod::Log;
namespace ChatColors = VoltMod::ChatColors;

/** Death, team changes and HUD updates dismiss center HTML, so the panel is redrawn far more often
 *  than its nominal five-second lifetime. Not configurable: slower blinks, faster only burns user
 *  messages. */
static constexpr int PanelRefreshMs = 100;

/** The deadline and the presence poll are both second-granularity, so they need nothing faster. */
static constexpr int DeadlineTickMs = 1000;

static bool IsValidLink(const std::string& link)
{
    if (link.rfind("https://", 0) != 0 && link.rfind("http://", 0) != 0)
        return false;
    // A real URL has no whitespace or HTML-significant characters; reject panel-injection attempts.
    return link.find_first_of(" \t\r\n<>\"'") == std::string::npos;
}

bool CheatCheckManager::StartCheck(int adminSlot, int targetSlot)
{
    if (!ValidSlot(adminSlot) || !ValidSlot(targetSlot))
        return false;

    auto& plrMgr = _rt.Players;
    auto* admin = plrMgr.Get(adminSlot);
    auto* target = plrMgr.Get(targetSlot);
    if (!admin || !target)
        return false;

    Controller targetCtrl = _rt.Entities.Controller(targetSlot);
    Pawn targetPawn = targetCtrl.GetPawn();
    const auto& cfg = _config.GetCheatCheck();

    // Re-call: keep the original movetype/team (target is already frozen/spectated, so reading now is stale).
    // PriorTeam is 0 (sentinel) unless we actually move them, so the restore decision survives a config reload.
    const bool wasActive = _checks[targetSlot].Active;
    const MoveType priorMove = wasActive ? _checks[targetSlot].PriorMoveType : targetPawn.Move();
    const int priorTeam = wasActive ? _checks[targetSlot].PriorTeam : (cfg.moveToSpectator ? int{targetPawn.Team} : 0);
    if (wasActive)
        ResetCheck(targetSlot);

    auto& pc = _checks[targetSlot];
    pc.Active = true;
    pc.AdminSlot = adminSlot;
    pc.AdminSteamId = admin->SteamId();
    pc.Mode = ParseMode(cfg.mode);
    pc.DeadlineSec = Time::Now() + cfg.timeoutSec;
    pc.ResolvedUrl.clear();
    pc.AwaitingUrl = false;
    pc.RequestSeq = _seq++;
    pc.PriorMoveType = priorMove;
    pc.PriorTeam = priorTeam;

    targetPawn.SetMove(MoveType::None);
    if (cfg.moveToSpectator)
        (void)targetCtrl.ChangeTeam(VoltMod::TeamSpectator);

    pc.DeadlineTimer = _rt.Scheduler.Repeat(DeadlineTickMs, [this, targetSlot] { Tick(targetSlot); });

    ResolveUrl(targetSlot);
    ShowPanel(targetSlot);
    _view.SendInstructions(targetSlot, pc);

    _chat.BroadcastAction("broadcast.cheatCheckCalled", admin->Name(), target->Name());
    return true;
}

void CheatCheckManager::ResolveUrl(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    const auto& cfg = _config.GetCheatCheck();

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
    auto* target = _rt.Players.Get(targetSlot);

    std::optional<RoomRequest> request;
    if (target)
    {
        auto* admin = _rt.Players.Get(pc.AdminSlot);
        request = BuildRoomRequest(_config.GetCheatCheck().websiteAutoRoom, target->SteamId(), target->Name(),
                                   pc.AdminSteamId, admin ? admin->Name() : std::string_view{});
    }

    if (!request)
    {
        // No endpoint to call (or the target vanished): fall back synchronously. StartCheck renders the
        // panel/chat afterward, so we only set state here (OnRoomFailed would double-send the instructions).
        FallbackToFixed(pc);
        return;
    }

    const uint64_t seq = pc.RequestSeq;
    VoltMod::Post(_rt.Http, std::move(*request), [this, targetSlot, seq](const VoltMod::HttpResult& result) {
        OnRoomResponse(targetSlot, seq, result);
    });
}

void CheatCheckManager::OnRoomResponse(int targetSlot, uint64_t seq, const VoltMod::HttpResult& result)
{
    if (!ValidSlot(targetSlot))
        return;
    auto& pc = _checks[targetSlot];
    if (!pc.Active || pc.RequestSeq != seq)  // stale: cancelled, expired, re-called, or slot reused
        return;

    const auto& roomCfg = _config.GetCheatCheck().websiteAutoRoom;
    if (auto urls = ParseRoomResponse(roomCfg, result))
    {
        pc.ResolvedUrl = std::move(urls->PlayerUrl);
        pc.AwaitingUrl = false;

        // An empty RoomCode keeps the per-tick poll gate on its fast path when polling is off.
        if (!roomCfg.presenceUrl.empty())
        {
            pc.RoomCode = std::move(urls->RoomCode);
            // First poll one interval out: the suspect can't have opened the link yet.
            pc.NextPollAtSec = Time::Now() + roomCfg.pollIntervalSec;
        }

        if (!urls->CheckerUrl.empty())
            RelayCheckerUrl(targetSlot, urls->CheckerUrl);
        _view.SendInstructions(targetSlot, pc);
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
        return std::format("{}{}", ChatColors::Red, _rt.Translations.Get("cheatCheck.apiFailed", adminSlot));
    });

    _view.SendInstructions(targetSlot, pc);
}

void CheatCheckManager::RelayCheckerUrl(int targetSlot, const std::string& checkerUrl)
{
    if (auto slot = ResolveAdminSlot(_checks[targetSlot]))
    {
        _chat.ReplyLink(*slot,
                        std::format("{}{}", ChatColors::Green, _rt.Translations.Get("cheatCheck.checkerUrl", *slot)),
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
    if (!pc.SuspectJoined && Time::Now() >= pc.DeadlineSec)
    {
        Expire(targetSlot);
        return;
    }

    PollPresenceIfDue(targetSlot);
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

    auto* caller = _rt.Players.Get(callerSlot);
    std::string name = caller ? caller->Name() : std::string();

    if (auto slot = ResolveAdminSlot(pc))
    {
        _chat.ReplyLink(*slot,
                        std::format("{}{} {}{}", ChatColors::Green,
                                    _rt.Translations.Get("cheatCheck.linkReceived", *slot), ChatColors::Default, name),
                        link);
    }

    return SubmitResult::Relayed;
}

void CheatCheckManager::FallbackToFixed(PendingCheck& pc)
{
    pc.AwaitingUrl = false;
    const auto& cfg = _config.GetCheatCheck();
    if (!cfg.fixedLink.url.empty())
        pc.ResolvedUrl = cfg.fixedLink.url;
}

std::optional<int> CheatCheckManager::ResolveAdminSlot(const PendingCheck& pc) const
{
    if (!_rt.Players.Get(VoltMod::PlayerRef{pc.AdminSlot, pc.AdminSteamId}))
        return std::nullopt;
    return pc.AdminSlot;
}

void CheatCheckManager::ReplyToAdmin(const PendingCheck& pc, const std::function<std::string()>& buildMessage)
{
    if (auto slot = ResolveAdminSlot(pc))
        _chat.Reply(*slot, buildMessage());
}

void CheatCheckManager::ShowPanel(int targetSlot)
{
    // Reads _checks live on every refresh, so the countdown and every state change (room created,
    // link submitted, suspect joined) reach the panel without anyone redrawing it.
    _panel.Show(targetSlot, PanelRefreshMs, [this](int slot) { return _view.PanelHtml(slot, _checks[slot]); });
}

void CheatCheckManager::ResetCheck(int targetSlot)
{
    _panel.Stop(targetSlot);
    _checks[targetSlot] = PendingCheck{};  // move-assign drops the deadline timer
}

bool CheatCheckManager::Cancel(int targetSlot)
{
    if (!ValidSlot(targetSlot) || !_checks[targetSlot].Active)
        return false;

    auto* target = _rt.Players.Get(targetSlot);
    std::string targetName = target ? target->Name() : std::string();

    const MoveType restore = _checks[targetSlot].PriorMoveType;
    const int restoreTeam = _checks[targetSlot].PriorTeam;
    ResetCheck(targetSlot);
    Unfreeze(targetSlot, restore, restoreTeam);

    _chat.BroadcastAction("broadcast.cheatCheckCleared", "", targetName);
    return true;
}

void CheatCheckManager::Expire(int targetSlot)
{
    const auto& cfg = _config.GetCheatCheck();
    const bool kick = cfg.autoKick;

    auto* target = _rt.Players.Get(targetSlot);
    std::string targetName = target ? target->Name() : std::string();

    const MoveType restore = _checks[targetSlot].PriorMoveType;
    const int restoreTeam = _checks[targetSlot].PriorTeam;
    ResetCheck(targetSlot);  // deactivate before the kick triggers disconnect cleanup

    if (kick)
        (void)_rt.Entities.Controller(targetSlot).Kick(cfg.kickReason);
    else
        Unfreeze(targetSlot, restore, restoreTeam);

    _chat.BroadcastAction("broadcast.cheatCheckTimedOut", "", targetName);
}

void CheatCheckManager::Unfreeze(int targetSlot, MoveType restoreMove, int restoreTeam)
{
    Controller controller = _rt.Entities.Controller(targetSlot);
    if (!controller)
        return;
    // restoreTeam is a real playing team (T/CT) only if we actually pulled them to spectator at start.
    if (restoreTeam >= VoltMod::TeamT)
        (void)controller.ChangeTeam(restoreTeam);
    controller.GetPawn().SetMove(restoreMove);
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
