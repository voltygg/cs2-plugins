#include "CheatCheckManager.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Config.hpp"
#include "../../Web/HttpClient.hpp"

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
    return link.rfind("https://", 0) == 0 || link.rfind("http://", 0) == 0;
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

    // Re-call: silently tear down the existing check (no "cleared" broadcast) before re-arming.
    if (_checks[targetSlot].Active)
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

    PlayerController(targetSlot).SetMoveType(MoveType::None);

    int interval = cfg.panelRefreshMs > 0 ? cfg.panelRefreshMs : 1000;
    pc.TickTimer = Scheduler::Instance().Repeat(interval, [this, targetSlot] { Tick(targetSlot); });

    ResolveUrl(targetSlot);
    SendChatInstructions(targetSlot);
    SendPanel(targetSlot);

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
    const auto& cfg = ConfigManager::Instance().GetCheatCheck();
    const auto& room = cfg.websiteAutoRoom;
    auto* target = PlayerManager::Instance().GetPlayerBySlot(targetSlot);

    if (!target || room.createRoomUrl.empty())
    {
        // No endpoint to call: fall back synchronously. StartCheck renders the panel/chat afterward,
        // so we only set state here (calling OnRoomFailed would double-send the instructions).
        pc.AwaitingUrl = false;
        if (!cfg.fixedLink.url.empty())
            pc.ResolvedUrl = cfg.fixedLink.url;
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
                SendChatInstructions(targetSlot);
                SendPanel(targetSlot);
                return;
            }
        }
    }

    OnRoomFailed(targetSlot);
}

void CheatCheckManager::OnRoomFailed(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    const auto& cfg = ConfigManager::Instance().GetCheatCheck();

    pc.AwaitingUrl = false;
    if (!cfg.fixedLink.url.empty())
        pc.ResolvedUrl = cfg.fixedLink.url;

    auto* adminPlayer = PlayerManager::Instance().GetPlayerBySlot(pc.AdminSlot);
    if (adminPlayer && adminPlayer->GetSteamID() == pc.AdminSteamId)
    {
        Translations::SlotScope scope(pc.AdminSlot);
        ChatService::Instance().Reply(
            pc.AdminSlot, std::format("{}{}", ChatColors::Red, Translations::Instance().Get("cheatCheck.apiFailed")));
    }

    SendChatInstructions(targetSlot);
    SendPanel(targetSlot);
}

void CheatCheckManager::RelayCheckerUrl(int targetSlot, const std::string& checkerUrl)
{
    auto& pc = _checks[targetSlot];
    auto* adminPlayer = PlayerManager::Instance().GetPlayerBySlot(pc.AdminSlot);
    if (!adminPlayer || adminPlayer->GetSteamID() != pc.AdminSteamId)
        return;

    Translations::SlotScope scope(pc.AdminSlot);
    auto& tr = Translations::Instance();
    ChatService::Instance().Reply(
        pc.AdminSlot,
        std::format("{}{} {}{}", ChatColors::Green, tr.Get("cheatCheck.checkerUrl"), ChatColors::Olive, checkerUrl));
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

    SendPanel(targetSlot);
}

void CheatCheckManager::SendPanel(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    if (!PlayerManager::Instance().GetPlayerBySlot(targetSlot))
        return;

    Translations::SlotScope scope(targetSlot);
    auto& tr = Translations::Instance();

    int64_t remain = pc.DeadlineSec - TimeUtils::Now();
    int remainSec = remain > 0 ? static_cast<int>(remain) : 0;

    std::string body;
    if (pc.AwaitingUrl)
        body = tr.Get("cheatCheck.creatingRoom");
    else if (pc.Mode == CheatCheckMode::PlayerProvided && pc.ResolvedUrl.empty())
        body = tr.Get("cheatCheck.provideLink");
    else if (!pc.ResolvedUrl.empty())
        body = std::format("{}<br>{}", tr.Get("cheatCheck.joinHere"), pc.ResolvedUrl);
    else
        body = tr.Get("cheatCheck.instructions");

    std::string html =
        std::format("<font color='#ff4040' size='5'>{}</font><br>{}<br><font color='#ffd040'>{}: {}s</font>",
                    tr.Get("cheatCheck.panelTitle"), body, tr.Get("cheatCheck.timeRemaining"), remainSec);

    if (ConfigManager::Instance().GetCheatCheck().autoKick)
        html += std::format("<br><font color='#ff8080'>{}</font>", tr.Get("cheatCheck.willKick"));

    MessageSystem::Instance().SendCenterHtml(targetSlot, html);
}

void CheatCheckManager::SendChatInstructions(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    Translations::SlotScope scope(targetSlot);
    auto& tr = Translations::Instance();
    auto& chat = ChatService::Instance();

    chat.Reply(targetSlot, std::format("{}{}", ChatColors::Red, tr.Get("cheatCheck.panelTitle")));

    if (pc.AwaitingUrl)
        chat.Reply(targetSlot, std::format("{}{}", ChatColors::Default, tr.Get("cheatCheck.creatingRoom")));
    else if (pc.Mode == CheatCheckMode::PlayerProvided && pc.ResolvedUrl.empty())
        chat.Reply(targetSlot, std::format("{}{}", ChatColors::Default, tr.Get("cheatCheck.provideLink")));
    else if (!pc.ResolvedUrl.empty())
        chat.Reply(targetSlot, std::format("{}{} {}{}", ChatColors::Default, tr.Get("cheatCheck.joinHere"),
                                           ChatColors::Olive, pc.ResolvedUrl));
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

    auto& plrMgr = PlayerManager::Instance();
    auto* adminPlayer = plrMgr.GetPlayerBySlot(pc.AdminSlot);
    auto* caller = plrMgr.GetPlayerBySlot(callerSlot);

    if (adminPlayer && adminPlayer->GetSteamID() == pc.AdminSteamId)
    {
        Translations::SlotScope scope(pc.AdminSlot);
        auto& tr = Translations::Instance();
        std::string name = caller ? caller->GetName() : std::string();
        ChatService::Instance().Reply(
            pc.AdminSlot, std::format("{}{} {}{}: {}{}", ChatColors::Green, tr.Get("cheatCheck.linkReceived"),
                                      ChatColors::Default, name, ChatColors::Olive, link));
    }

    SendPanel(callerSlot);
    return SubmitResult::Relayed;
}

void CheatCheckManager::ResetCheck(int targetSlot)
{
    auto& pc = _checks[targetSlot];
    if (pc.TickTimer)
        Scheduler::Instance().Cancel(pc.TickTimer);
    MessageSystem::Instance().ClearCenterHtml(targetSlot);
    pc = PendingCheck{};
}

void CheatCheckManager::Cancel(int targetSlot)
{
    if (!ValidSlot(targetSlot) || !_checks[targetSlot].Active)
        return;

    auto* target = PlayerManager::Instance().GetPlayerBySlot(targetSlot);
    std::string targetName = target ? target->GetName() : std::string();

    ResetCheck(targetSlot);
    Unfreeze(targetSlot);

    ChatService::Instance().BroadcastAction("broadcast.cheatCheckCleared", "", targetName);
}

void CheatCheckManager::Expire(int targetSlot)
{
    const auto& cfg = ConfigManager::Instance().GetCheatCheck();
    const bool kick = cfg.autoKick;
    const std::string kickReason = cfg.kickReason;

    auto* target = PlayerManager::Instance().GetPlayerBySlot(targetSlot);
    std::string targetName = target ? target->GetName() : std::string();

    ResetCheck(targetSlot);  // deactivate before the kick triggers disconnect cleanup

    if (kick)
        PlayerController(targetSlot).Kick(kickReason.c_str());
    else
        Unfreeze(targetSlot);

    ChatService::Instance().BroadcastAction("broadcast.cheatCheckTimedOut", "", targetName);
}

void CheatCheckManager::Unfreeze(int targetSlot)
{
    PlayerController pc(targetSlot);
    if (pc.IsValid())
        pc.SetMoveType(MoveType::Walk);
}

bool CheatCheckManager::IsPending(int targetSlot) const
{
    return ValidSlot(targetSlot) && _checks[targetSlot].Active;
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
        Unfreeze(slot);
        ResetCheck(slot);
    }
}

}  // namespace AdminSystem::Admin::CheatCheck
