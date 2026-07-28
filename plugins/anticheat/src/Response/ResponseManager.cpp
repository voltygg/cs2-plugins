#include "ResponseManager.hpp"

#include "Managers.hpp"

#include <CS2Kit/Utils/Log.hpp>
#include <algorithm>
#include <format>

using CS2Kit::Core::Engine;
namespace Log = CS2Kit::Utils::Log;

namespace Anticheat
{

namespace
{
// The console line has a hard length limit, and a DLL-injection evidence string can run to
// hundreds of characters; the full text is already in the log and the webhook.
constexpr size_t MaxReasonLength = 200;

// Reasons travel through ExecuteServerCommand; keep them free of anything the console parser
// could read as command structure.
std::string SanitizeReason(std::string_view reason)
{
    std::string out;
    out.reserve(reason.size());
    for (char c : reason)
        if (c != '"' && c != ';' && c != '\n' && c != '\r')
            out += c;
    if (out.size() > MaxReasonLength)
        out.resize(MaxReasonLength);
    return out;
}
}  // namespace

void ResponseManager::Initialize()
{
    _latch.Reset();
}

void ResponseManager::OnSlotChanged(int slot)
{
    _latch.Clear(slot);
}

void ResponseManager::ResetAll()
{
    _latch.Reset();
}

Mode ResponseManager::CurrentMode() const
{
    return ParseMode(App().Config.Get().anticheat.mode);
}

bool ResponseManager::IsWhitelisted(int64_t steamId) const
{
    const auto& ids = App().Config.Get().anticheat.whitelistSteamIds;
    return std::find(ids.begin(), ids.end(), steamId) != ids.end();
}

void ResponseManager::Handle(int slot, const Finding& finding)
{
    auto* player = Engine().Players.GetPlayerBySlot(slot);
    const std::string name = player ? player->GetName() : std::string("<unknown>");
    const int64_t steamId = player ? player->GetSteamID() : 0;

    const FunnelDecision decision = Decide({
        .SteamId = steamId,
        .Whitelisted = IsWhitelisted(steamId),
        .CurrentMode = CurrentMode(),
        .KickOnly = finding.KickOnly,
        .Issued = _latch.Level(slot),
    });

    Log::Warn("[AC] {} on {} ({}) -> {}: {}", DisplayName(finding.Kind), name, steamId, OutcomeName(decision.Outcome),
              finding.Evidence);
    _reporter.Report(slot, name, steamId, finding, decision.Outcome);

    if (decision.SendAlert &&
        _alertThrottle.TryAcquire({steamId, static_cast<int>(finding.Kind)}, CS2Kit::TimeUtils::Now()))
        Engine().ConVars.ExecuteServerCommand(
            std::format("as_ac_alert {} {} 1", steamId, TokenName(finding.Kind)).c_str());

    if (decision.Apply == PunishmentLevel::None || !_latch.Raise(slot, decision.Apply))
        return;

    const std::string reason =
        SanitizeReason(std::format("AntiCheat: {} ({})", DisplayName(finding.Kind), finding.Evidence));
    if (decision.Apply == PunishmentLevel::Kick)
    {
        // A finding can surface from inside an engine hook on the client object itself (the convar
        // query reply hook), and kicking disconnects that very client mid-virtual-call. Defer one
        // tick, then re-resolve the slot in case its player left and somebody else took it.
        Engine().Scheduler.NextTick([slot, steamId, reason] {
            if (Engine().Players.GetPlayerBySlotIfSteamId(slot, steamId))
                CS2Kit::PlayerController(slot).Kick(reason.c_str());
        });
        return;
    }
    Engine().ConVars.ExecuteServerCommand(
        std::format("as_ac_ban {} {} {}", steamId, App().Config.Get().anticheat.banDurationSec, reason).c_str());
}

}  // namespace Anticheat
