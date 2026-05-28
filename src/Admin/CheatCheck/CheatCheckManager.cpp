#include "CheatCheckManager.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Config.hpp"
#include "../../Web/HttpClient.hpp"
#include "CheatCheckView.hpp"

#include <CS2Kit/Core/Scheduler.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Sdk/UserMessage.hpp>
#include <CS2Kit/Utils/ChatColors.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>
#include <nlohmann/json.hpp>

namespace AdminSystem::Admin::CheatCheck
{

using AdminSystem::Core::ChatService;
using AdminSystem::Core::ConfigManager;
using CS2Kit::Core::Scheduler;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Sdk::MessageSystem;
using CS2Kit::Sdk::MoveType;
using CS2Kit::Sdk::PlayerController;
using CS2Kit::Utils::TimeUtils;
using CS2Kit::Utils::Translations;
namespace ChatColors = CS2Kit::Utils::ChatColors;

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

    auto& plrMgr = PlayerManager::Instance();
    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!admin || !target)
        return false;

    PlayerController targetCtrl(targetSlot);

    // Re-call: keep the original movetype (target is already frozen, so reading it now yields None).
    const bool wasActive = _checks[targetSlot].Active;
    const MoveType priorMove = wasActive ? _checks[targetSlot].PriorMoveType : targetCtrl.GetMoveType();
    if (wasActive)
        ResetCheck(targetSlot);

    const auto& cfg = ConfigManager::Instance().GetCheatCheck();

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

    targetCtrl.SetMoveType(MoveType::None);

    int interval = cfg.panelRefreshMs > 0 ? cfg.panelRefreshMs : 1000;
    pc.TickTimer = Scheduler::Instance().Repeat(interval, [this, targetSlot] { Tick(targetSlot); });

    ResolveUrl(targetSlot);
    View::Render(targetSlot, pc);

    ChatService::Instance().BroadcastAction("broadcast.cheatCheckCalled", admin->GetName(), target->GetName());
    return true;
}

void CheatCheckManager::ResolveUrl(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    const auto& cfg = ConfigManager::Instance().GetCheatCheck();

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
    const auto& room = ConfigManager::Instance().GetCheatCheck().websiteAutoRoom;
    auto* target = PlayerManager::Instance().GetPlayerBySlot(targetSlot);

    if (!target || room.createRoomUrl.empty())
    {
        // No endpoint to call: fall back synchronously. StartCheck renders the panel/chat afterward,
        // so we only set state here (calling OnRoomFailed would double-send the instructions).
        FallbackToFixed(pc);
        return;
    }

    const nlohmann::json body = {
        {"steamId", std::to_string(target->GetSteamID())},
        {"playerName", target->GetName()},
        {"adminSteamId", std::to_string(pc.AdminSteamId)},
    };

    std::vector<std::string> headers = {"Content-Type: application/json"};
    if (!room.apiKey.empty())
        headers.push_back("Authorization: Bearer " + room.apiKey);

    const uint64_t seq = pc.RequestSeq;
    Web::HttpClient::Instance().Post(
        room.createRoomUrl, body.dump(), std::move(headers), room.timeoutMs,
        [this, targetSlot, seq](const Web::HttpResult& result) { OnRoomResponse(targetSlot, seq, result); });
}

void CheatCheckManager::OnRoomResponse(int targetSlot, uint64_t seq, const Web::HttpResult& result)
{
    if (!ValidSlot(targetSlot))
        return;
    auto& pc = _checks[targetSlot];
    if (!pc.Active || pc.RequestSeq != seq)  // stale: cancelled, expired, re-called, or slot reused
        return;

    if (result.Ok && result.StatusCode >= 200 && result.StatusCode < 300)
    {
        auto json = nlohmann::json::parse(result.Body, nullptr, /*allow_exceptions=*/false);
        if (json.is_object())
        {
            std::string playerUrl = json.value("playerUrl", std::string());
            std::string checkerUrl = json.value("checkerUrl", std::string());
            if (!playerUrl.empty())
            {
                pc.ResolvedUrl = std::move(playerUrl);
                pc.AwaitingUrl = false;
                if (!checkerUrl.empty())
                    RelayCheckerUrl(targetSlot, checkerUrl);
                View::Render(targetSlot, pc);
                return;
            }
        }
    }

    OnRoomFailed(targetSlot);
}

void CheatCheckManager::OnRoomFailed(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    FallbackToFixed(pc);

    ReplyToAdmin(
        pc, [] { return std::format("{}{}", ChatColors::Red, Translations::Instance().Get("cheatCheck.apiFailed")); });

    View::Render(targetSlot, pc);
}

void CheatCheckManager::RelayCheckerUrl(int targetSlot, const std::string& checkerUrl)
{
    ReplyToAdmin(_checks[targetSlot], [&checkerUrl] {
        auto& tr = Translations::Instance();
        return std::format("{}{} {}{}", ChatColors::Green, tr.Get("cheatCheck.checkerUrl"), ChatColors::Olive,
                           checkerUrl);
    });
}

void CheatCheckManager::Tick(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    if (!pc.Active)
        return;

    if (TimeUtils::Now() >= pc.DeadlineSec)
    {
        Expire(targetSlot);
        return;
    }

    View::RenderPanel(targetSlot, pc);
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

    auto* caller = PlayerManager::Instance().GetPlayerBySlot(callerSlot);
    std::string name = caller ? caller->GetName() : std::string();
    ReplyToAdmin(pc, [&] {
        auto& tr = Translations::Instance();
        return std::format("{}{} {}{}: {}{}", ChatColors::Green, tr.Get("cheatCheck.linkReceived"), ChatColors::Default,
                           name, ChatColors::Olive, link);
    });

    View::RenderPanel(callerSlot, pc);
    return SubmitResult::Relayed;
}

void CheatCheckManager::FallbackToFixed(PendingCheck& pc)
{
    pc.AwaitingUrl = false;
    const auto& cfg = ConfigManager::Instance().GetCheatCheck();
    if (!cfg.fixedLink.url.empty())
        pc.ResolvedUrl = cfg.fixedLink.url;
}

void CheatCheckManager::ReplyToAdmin(const PendingCheck& pc, const std::function<std::string()>& buildMessage)
{
    auto* adminPlayer = PlayerManager::Instance().GetPlayerBySlot(pc.AdminSlot);
    if (!adminPlayer || adminPlayer->GetSteamID() != pc.AdminSteamId)
        return;

    Translations::SlotScope scope(pc.AdminSlot);
    ChatService::Instance().Reply(pc.AdminSlot, buildMessage());
}

void CheatCheckManager::ResetCheck(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    if (pc.TickTimer)
        Scheduler::Instance().Cancel(pc.TickTimer);
    MessageSystem::Instance().ClearCenterHtml(targetSlot);
    pc = PendingCheck{};
}

bool CheatCheckManager::Cancel(int targetSlot)
{
    if (!ValidSlot(targetSlot) || !_checks[targetSlot].Active)
        return false;

    auto* target = PlayerManager::Instance().GetPlayerBySlot(targetSlot);
    std::string targetName = target ? target->GetName() : std::string();

    const MoveType restore = _checks[targetSlot].PriorMoveType;
    ResetCheck(targetSlot);
    Unfreeze(targetSlot, restore);

    ChatService::Instance().BroadcastAction("broadcast.cheatCheckCleared", "", targetName);
    return true;
}

void CheatCheckManager::Expire(int targetSlot)
{
    const auto& cfg = ConfigManager::Instance().GetCheatCheck();
    const bool kick = cfg.autoKick;

    auto* target = PlayerManager::Instance().GetPlayerBySlot(targetSlot);
    std::string targetName = target ? target->GetName() : std::string();

    const MoveType restore = _checks[targetSlot].PriorMoveType;
    ResetCheck(targetSlot);  // deactivate before the kick triggers disconnect cleanup

    if (kick)
        PlayerController(targetSlot).Kick(cfg.kickReason.c_str());
    else
        Unfreeze(targetSlot, restore);

    ChatService::Instance().BroadcastAction("broadcast.cheatCheckTimedOut", "", targetName);
}

void CheatCheckManager::Unfreeze(int targetSlot, MoveType restore)
{
    PlayerController pc(targetSlot);
    if (pc.IsValid())
        pc.SetMoveType(restore);
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
        Unfreeze(slot, _checks[slot].PriorMoveType);
        ResetCheck(slot);
    }
}

}  // namespace AdminSystem::Admin::CheatCheck
