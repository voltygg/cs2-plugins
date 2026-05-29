#include "CheatCheckView.hpp"
#include "../../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../../Core/ChatService.hpp"
#include "../../Core/Config.hpp"

#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/UserMessage.hpp>
#include <CS2Kit/Utils/ChatColors.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <format>
#include <string>

using CS2Kit::Core::Kit;

namespace AdminSystem::Admin::CheatCheck::View
{

using AdminSystem::Core::ChatService;
using AdminSystem::Core::ConfigManager;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Sdk::MessageSystem;
using CS2Kit::Utils::TimeUtils;
namespace ChatColors = CS2Kit::Utils::ChatColors;

namespace
{

enum class PanelState
{
    CreatingRoom,  // awaiting async room creation
    ProvideLink,   // playerProvided mode, suspect hasn't submitted yet
    HasUrl,        // a URL is ready to show
    Generic,       // no URL and nothing pending
};

PanelState PanelStateFor(const PendingCheck& pc)
{
    if (pc.AwaitingUrl)
        return PanelState::CreatingRoom;
    if (pc.Mode == CheatCheckMode::PlayerProvided && pc.ResolvedUrl.empty())
        return PanelState::ProvideLink;
    if (!pc.ResolvedUrl.empty())
        return PanelState::HasUrl;
    return PanelState::Generic;
}

std::string EscapeHtml(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for (char c : text)
    {
        switch (c)
        {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '"':
            out += "&quot;";
            break;
        case '\'':
            out += "&#39;";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

}  // namespace

void RenderPanel(int slot, const PendingCheck& pc)
{
    if (!Kit().Players.GetPlayerBySlot(slot))
        return;

    auto& tr = Kit().Translations;

    int64_t remain = pc.DeadlineSec - TimeUtils::Now();
    int remainSec = remain > 0 ? static_cast<int>(remain) : 0;

    std::string body;
    switch (PanelStateFor(pc))
    {
    case PanelState::CreatingRoom:
        body = tr.Get("cheatCheck.creatingRoom", slot);
        break;
    case PanelState::ProvideLink:
        body = tr.Get("cheatCheck.provideLink", slot);
        break;
    case PanelState::HasUrl:
        body = std::format("{}<br>{}", tr.Get("cheatCheck.joinHere", slot), EscapeHtml(pc.ResolvedUrl));
        break;
    case PanelState::Generic:
        body = tr.Get("cheatCheck.instructions", slot);
        break;
    }

    std::string html =
        std::format("<font color='#ff4040' size='5'>{}</font><br>{}<br><font color='#ffd040'>{}: {}s</font>",
                    tr.Get("cheatCheck.panelTitle", slot), body, tr.Get("cheatCheck.timeRemaining", slot), remainSec);

    if (Sys().Config.GetCheatCheck().autoKick)
        html += std::format("<br><font color='#ff8080'>{}</font>", tr.Get("cheatCheck.willKick", slot));

    Kit().Messages.SendCenterHtml(slot, html);
}

void Render(int slot, const PendingCheck& pc)
{
    auto& tr = Kit().Translations;
    auto& chat = Sys().Chat;

    chat.Reply(slot, std::format("{}{}", ChatColors::Red, tr.Get("cheatCheck.panelTitle", slot)));

    switch (PanelStateFor(pc))
    {
    case PanelState::CreatingRoom:
        chat.Reply(slot, std::format("{}{}", ChatColors::Default, tr.Get("cheatCheck.creatingRoom", slot)));
        break;
    case PanelState::ProvideLink:
        chat.Reply(slot, std::format("{}{}", ChatColors::Default, tr.Get("cheatCheck.provideLink", slot)));
        break;
    case PanelState::HasUrl:
        chat.Reply(slot, std::format("{}{} {}{}", ChatColors::Default, tr.Get("cheatCheck.joinHere", slot),
                                     ChatColors::Olive, pc.ResolvedUrl));
        break;
    case PanelState::Generic:
        break;
    }

    RenderPanel(slot, pc);
}

}  // namespace AdminSystem::Admin::CheatCheck::View
