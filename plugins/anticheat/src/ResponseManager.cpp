#include "ResponseManager.hpp"

#include "Managers.hpp"
#include "PlayerMonitor.hpp"

#include <CS2Kit/Utils/Log.hpp>
#include <format>

using CS2Kit::Core::Engine;
namespace Log = CS2Kit::Utils::Log;

namespace Anticheat
{

namespace
{
// Ban reasons travel through ExecuteServerCommand; keep them free of anything
// the console parser could treat as command structure.
std::string SanitizeReason(std::string_view reason)
{
    std::string out;
    out.reserve(reason.size());
    for (char c : reason)
        if (c != '"' && c != ';' && c != '\n' && c != '\r')
            out += c;
    return out;
}
}  // namespace

void ResponseManager::Initialize()
{
    _slots.BindReset();
}

Mode ResponseManager::CurrentMode() const
{
    const std::string& mode = App().Config.Get().anticheat.mode;
    if (mode == "ban")
        return Mode::Ban;
    if (mode == "alert")
        return Mode::Alert;
    return Mode::Observe;
}

void ResponseManager::Handle(int slot, const Detection& detection)
{
    auto* player = Engine().Players.GetPlayerBySlot(slot);
    if (!player)
        return;

    auto& response = _slots[slot];
    auto& score = response.Scores[detection.Detector];
    score.SetDecayRate(detection.DecayPerSec);

    double now = NowSeconds();
    score.Add(detection.ScoreAdd, now);
    float value = score.Value(now);

    Log::Warn("[AC] {} ({}) {}: +{:.0f} -> {:.0f}: {}", player->GetName(), player->GetSteamID(), detection.Detector,
              detection.ScoreAdd, value, detection.Detail);

    const auto& settings = App().Config.Get().anticheat;
    if (settings.debug.broadcastDetections)
        Engine().Messages.Broadcast(std::format("[AC] {} flagged {}: {:.0f}/{:.0f} ({})", player->GetName(),
                                                detection.Detector, value, detection.BanScore, detection.Detail));

    // ObserveOnly buckets (unconfirmed anomalies) log/broadcast but never escalate.
    Mode mode = CurrentMode();
    if (detection.ObserveOnly || mode == Mode::Observe)
        return;

    if (value >= detection.AlertScore && now - response.LastAlert >= settings.alertCooldownSec)
    {
        response.LastAlert = now;
        Engine().ConVars.ExecuteServerCommand(
            std::format("as_ac_alert {} {} {:.0f}", player->GetSteamID(), detection.Detector, value).c_str());
    }

    if (mode == Mode::Ban && !response.BanIssued && value >= detection.BanScore &&
        detection.EventsInWindow >= detection.MinEvents)
    {
        response.BanIssued = true;
        Log::Warn("[AC] auto-ban {} ({}): {} score {:.0f} with {} events.", player->GetName(), player->GetSteamID(),
                  detection.Detector, value, detection.EventsInWindow);
        if (settings.debug.dryRunBans)
        {
            Engine().Messages.Broadcast(std::format("[AC] WOULD BAN {} ({}) - {} score {:.0f}, {} events",
                                                    player->GetName(), player->GetSteamID(), detection.Detector, value,
                                                    detection.EventsInWindow));
        }
        else
        {
            std::string reason = std::format("{} [{}]", SanitizeReason(settings.ban.reason), detection.Detector);
            Engine().ConVars.ExecuteServerCommand(
                std::format("as_ac_ban {} {} {}", player->GetSteamID(), settings.ban.durationSec, reason).c_str());
        }
    }
}

void ResponseManager::DumpStatus()
{
    double now = NowSeconds();
    bool any = false;
    for (auto* player : Engine().Players.GetAllPlayers())
    {
        auto& response = _slots[player->GetSlot()];
        std::string line;
        for (auto& [detector, score] : response.Scores)
        {
            float value = score.Value(now);
            if (value <= 0.0f)
                continue;
            line += std::format(" {}={:.0f}", detector, value);
        }
        if (line.empty())
            continue;
        any = true;
        Log::Info("[AC] {} ({}){}{}", player->GetName(), player->GetSteamID(), line,
                  response.BanIssued ? " [ban issued]" : "");
    }
    if (!any)
        Log::Info("[AC] no active scores.");
}

}  // namespace Anticheat
