#include "Response/ResponseManager.hpp"

#include "App.hpp"

#include <Contracts/IAdminActions.hpp>
#include <VoltMod/Core/Log.hpp>
#include <algorithm>
#include <cmath>
#include <format>

namespace Log = VoltMod::Log;

namespace Anticheat
{

// The reason is also the kick message and the chat broadcast, and DLL-injection evidence runs
// long. The full text is already in the log and the webhook.
static constexpr size_t MaxReasonLength = 200;

/** admin-system's cross-plugin surface, or nullptr when that plugin is not loaded. */
static Contracts::IAdminActions* AdminActions(VoltMod::Runtime& rt)
{
    return rt.Exchange.Get<Contracts::IAdminActions>();
}

static std::string TrimReason(std::string_view reason)
{
    return std::string(reason.substr(0, std::min(reason.size(), MaxReasonLength)));
}

Mode ResponseManager::CurrentMode() const
{
    return ParseMode(_config.Get().anticheat.mode);
}

bool ResponseManager::IsWhitelisted(int64_t steamId) const
{
    const auto& ids = _config.Get().anticheat.whitelistSteamIds;
    return std::find(ids.begin(), ids.end(), steamId) != ids.end();
}

void ResponseManager::Handle(int slot, const Finding& finding)
{
    auto* player = _rt.Players.Get(slot);
    const std::string name = player ? player->Name() : std::string("<unknown>");
    const int64_t steamId = player ? player->SteamId() : 0;

    const ResponseDecision decision = Decide({
        .SteamId = steamId,
        .Whitelisted = IsWhitelisted(steamId),
        .CurrentMode = CurrentMode(),
        .Level = finding.Level,
        .KickOnly = finding.KickOnly,
        .Issued = _issued.Level(steamId),
    });

    if (decision.SendAlert)
    {
        if (auto* admin = AdminActions(_rt))
            admin->AlertAdmins(steamId, TokenName(finding.Kind), std::lround(finding.Suspicion * 100.0f));
    }

    if (decision.Apply == PunishmentLevel::None)
    {
        Record(slot, name, steamId, finding, decision.Outcome);
        return;
    }

    const std::string reason =
        TrimReason(std::format("AntiCheat: {} ({})", DisplayName(finding.Kind), finding.Evidence));
    if (decision.Apply == PunishmentLevel::Kick)
    {
        // A finding can surface inside an engine hook on the client itself, where kicking would
        // disconnect it mid-virtual-call.
        _pendingKick[slot] = _rt.Scheduler.NextTick([this, slot, steamId, name, finding, reason] {
            Record(slot, name, steamId, finding, ApplyKick(slot, steamId, reason));
        });
        return;
    }
    Record(slot, name, steamId, finding, ApplyBan(steamId, reason));
}

ResponseOutcome ResponseManager::ApplyKick(int slot, int64_t steamId, const std::string& reason)
{
    if (!_rt.Players.Get(VoltMod::PlayerRef{slot, steamId}))
        return ResponseOutcome::TargetGone;
    if (!_rt.Entities.Controller(slot).Kick(reason))
        return ResponseOutcome::KickFailed;

    _issued.Raise(steamId, PunishmentLevel::Kick);
    return ResponseOutcome::KickIssued;
}

ResponseOutcome ResponseManager::ApplyBan(int64_t steamId, const std::string& reason)
{
    auto* admin = AdminActions(_rt);
    if (!admin)
    {
        // Make the missing optional dependency visible, and leave the ban retryable.
        Log::Warn("[AC] cannot ban {}: admin-system is not loaded (no {}).", steamId,
                  Contracts::IAdminActions::InterfaceName);
        return ResponseOutcome::BanUnavailable;
    }

    if (const auto result = admin->Ban(steamId, _config.Get().anticheat.banDurationSec, reason);
        result != Contracts::BanResult::Ok)
    {
        Log::Warn("[AC] ban for {} rejected by admin-system (code {}).", steamId, static_cast<int>(result));
        return ResponseOutcome::BanRejected;
    }

    _issued.Raise(steamId, PunishmentLevel::Ban);
    return ResponseOutcome::BanIssued;
}

void ResponseManager::Record(int slot, const std::string& name, int64_t steamId, const Finding& finding,
                             ResponseOutcome outcome)
{
    Log::Warn("[AC] {} on {} ({}) {} -> {}: {}", DisplayName(finding.Kind), name, steamId,
              ConfidenceName(finding.Level), OutcomeName(outcome), finding.Evidence);
    _reporter.Report(slot, name, steamId, finding, outcome);
}

}  // namespace Anticheat
