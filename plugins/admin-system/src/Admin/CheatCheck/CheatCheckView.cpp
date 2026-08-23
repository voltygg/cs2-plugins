#include "CheatCheckView.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Config.hpp"
#include "../../Core/Managers.hpp"

#include <CS2Kit/App/Services.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/UserMessage.hpp>
#include <CS2Kit/Utils/ChatColors.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <format>
#include <string>

using CS2Kit::App::Engine;

namespace AdminSystem::Admin::CheatCheck::View
{

using AdminSystem::Core::ChatService;
using AdminSystem::Core::ConfigManager;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Sdk::MessageSystem;
using CS2Kit::Utils::StringUtils;
using CS2Kit::Utils::TimeUtils;
namespace ChatColors = CS2Kit::Utils::ChatColors;

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

void RenderPanel(int slot, const PendingCheck& pc)
{
    if (!Engine().Players.GetPlayerBySlot(slot))
    {
        return;
    }

    auto& tr = Engine().Utils.Translations;

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

    const auto& cfg = App().Config.GetCheatCheck();

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

    Engine().Sdk.Messages.SendCenterHtml(slot, html);
}

void Render(int slot, const PendingCheck& pc)
{
    auto& tr = Engine().Utils.Translations;
    auto& chat = App().Chat;

    chat.Reply(slot, std::format("{}{}", ChatColors::Red, tr.Get("cheatCheck.panelTitle", slot)));

    switch (PanelStateFor(pc))
    {
    case PanelState::Joined:
        chat.Reply(slot, std::format("{}{}", ChatColors::Green, tr.Get("cheatCheck.joinedPanel", slot)));
        break;
    case PanelState::CreatingRoom:
        chat.Reply(slot, std::format("{}{}", ChatColors::Default, tr.Get("cheatCheck.creatingRoom", slot)));
        break;
    case PanelState::ProvideLink:
        chat.Reply(slot, std::format("{}{}", ChatColors::Default, tr.Get("cheatCheck.provideLink", slot)));
        break;
    case PanelState::HasUrl:
        chat.ReplyLink(slot, std::format("{}{}", ChatColors::Default, tr.Get("cheatCheck.joinHere", slot)),
                       pc.ResolvedUrl);
        break;
    case PanelState::Generic:
        break;
    }

    RenderPanel(slot, pc);
}

}  // namespace AdminSystem::Admin::CheatCheck::View
