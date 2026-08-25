#include "CheatCheckView.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Config.hpp"

#include <VoltMod/Core/ChatColors.hpp>
#include <VoltMod/Core/StringUtils.hpp>
#include <VoltMod/Core/TimeUtils.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <VoltMod/Sdk/Messaging/UserMessage.hpp>
#include <format>
#include <string>

namespace AdminSystem::Admin::CheatCheck
{

using AdminSystem::Core::ChatService;
using AdminSystem::Core::ConfigManager;
using VoltMod::Core::StringUtils;
using VoltMod::Core::TimeUtils;
using VoltMod::Players::PlayerManager;
using VoltMod::Sdk::MessageSystem;
namespace ChatColors = VoltMod::Core::ChatColors;

namespace
{

enum class PanelState
{
    Joined,        // suspect is in the check room; countdown paused
    CreatingRoom,  // awaiting async room creation
    ProvideLink,   // playerProvided mode, suspect hasn't submitted yet
    HasUrl,        // a URL is ready to show
    Generic,       // no URL and nothing pending
};

PanelState PanelStateFor(const PendingCheck& pc)
{
    if (pc.SuspectJoined)
        return PanelState::Joined;
    if (pc.AwaitingUrl)
        return PanelState::CreatingRoom;
    if (pc.Mode == CheatCheckMode::PlayerProvided && pc.ResolvedUrl.empty())
        return PanelState::ProvideLink;
    if (!pc.ResolvedUrl.empty())
        return PanelState::HasUrl;
    return PanelState::Generic;
}

}  // namespace

void CheatCheckView::RenderPanel(int slot, const PendingCheck& pc) const
{
    if (!_rt.Players.GetPlayerBySlot(slot))
    {
        return;
    }

    auto& tr = _rt.Translations;

    int64_t remain = pc.DeadlineSec - TimeUtils::Now();
    int remainSec = remain > 0 ? static_cast<int>(remain) : 0;

    const PanelState state = PanelStateFor(pc);

    std::string body;
    switch (state)
    {
    case PanelState::Joined:
        body = tr.Get("cheatCheck.joinedPanel", slot);
        break;
    case PanelState::CreatingRoom:
        body = tr.Get("cheatCheck.creatingRoom", slot);
        break;
    case PanelState::ProvideLink:
        body = tr.Get("cheatCheck.provideLink", slot);
        break;
    case PanelState::HasUrl:
        body = std::format("{}<br>{}", tr.Get("cheatCheck.joinHere", slot), StringUtils::EscapeHtml(pc.ResolvedUrl));
        break;
    case PanelState::Generic:
        body = tr.Get("cheatCheck.instructions", slot);
        break;
    }

    const auto& cfg = _config.GetCheatCheck();

    // Operator-configured (trusted) banner image rendered atop the panel; CS2 fetches it client-side.
    std::string html;
    if (!cfg.bannerImageUrl.empty())
    {
        html = std::format("<img src='{}' width='{}' height='{}'/><br>", cfg.bannerImageUrl, cfg.bannerWidth,
                           cfg.bannerHeight);
    }

    // Joined pauses the countdown: a calm status line replaces the timer and the kick warning.
    std::string statusLine =
        state == PanelState::Joined
            ? std::format("<font color='#40ff70'>{}</font>", tr.Get("cheatCheck.joinedStatus", slot))
            : std::format("<font color='#ffd040'>{}: {}s</font>", tr.Get("cheatCheck.timeRemaining", slot), remainSec);

    html += std::format("<font color='#ff4040' size='5'>{}</font><br>{}<br>{}", tr.Get("cheatCheck.panelTitle", slot),
                        body, statusLine);

    if (cfg.autoKick && state != PanelState::Joined)
    {
        html += std::format("<br><font color='#ff8080'>{}</font>", tr.Get("cheatCheck.willKick", slot));
    }

    _rt.Messages.SendCenterHtml(slot, html);
}

void CheatCheckView::Render(int slot, const PendingCheck& pc) const
{
    auto& tr = _rt.Translations;

    _chat.Reply(slot, std::format("{}{}", ChatColors::Red, tr.Get("cheatCheck.panelTitle", slot)));

    switch (PanelStateFor(pc))
    {
    case PanelState::Joined:
        _chat.Reply(slot, std::format("{}{}", ChatColors::Green, tr.Get("cheatCheck.joinedPanel", slot)));
        break;
    case PanelState::CreatingRoom:
        _chat.Reply(slot, std::format("{}{}", ChatColors::Default, tr.Get("cheatCheck.creatingRoom", slot)));
        break;
    case PanelState::ProvideLink:
        _chat.Reply(slot, std::format("{}{}", ChatColors::Default, tr.Get("cheatCheck.provideLink", slot)));
        break;
    case PanelState::HasUrl:
        _chat.ReplyLink(slot, std::format("{}{}", ChatColors::Default, tr.Get("cheatCheck.joinHere", slot)),
                        pc.ResolvedUrl);
        break;
    case PanelState::Generic:
        break;
    }

    RenderPanel(slot, pc);
}

}  // namespace AdminSystem::Admin::CheatCheck
