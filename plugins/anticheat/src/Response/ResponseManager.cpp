#include "ResponseManager.hpp"

#include "App.hpp"

#include <Contracts/IAdminActions.hpp>
#include <VoltMod/Core/Log.hpp>
#include <algorithm>
#include <format>

namespace Log = VoltMod::Core::Log;

namespace Anticheat
{

namespace
{
// The reason is also the kick message and the chat broadcast, and DLL-injection evidence runs
// long. The full text is already in the log and the webhook.
constexpr size_t MaxReasonLength = 200;

/** admin-system's cross-plugin surface, or nullptr when that plugin is not loaded. */
Contracts::IAdminActions* AdminActions(VoltMod::Runtime& rt)
{
    return rt.Exchange.Get<Contracts::IAdminActions>();
}

std::string TrimReason(std::string_view reason)
{
    return std::string(reason.substr(0, std::min(reason.size(), MaxReasonLength)));
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

void ResponseManager::Reset()
{
    _latch.Reset();
    // Keyed by SteamID, so without this the map grows for the lifetime of the server.
    _alertThrottle.Prune(VoltMod::Time::Now(), AlertThrottleSec);
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
    auto* player = _rt.Players.GetPlayerBySlot(slot);
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
        _alertThrottle.TryAcquire({steamId, static_cast<int>(finding.Kind)}, VoltMod::Time::Now()))
    {
        if (auto* admin = AdminActions(_rt))
            admin->AlertAdmins(steamId, TokenName(finding.Kind), 1);
    }

    if (decision.Apply == PunishmentLevel::None || !_latch.Raise(slot, decision.Apply))
        return;

    const std::string reason =
        TrimReason(std::format("AntiCheat: {} ({})", DisplayName(finding.Kind), finding.Evidence));
    if (decision.Apply == PunishmentLevel::Kick)
    {
        // A finding can surface from inside an engine hook on the client itself (the convar query
        // reply), where kicking would disconnect it mid-virtual-call. Defer a tick, then re-resolve
        // the slot in case its player left and somebody else took it.
        _rt.Scheduler.NextTick([&players = _rt.Players, &entities = _rt.Entities, slot, steamId, reason] {
            if (players.GetPlayerBySlotIfSteamId(slot, steamId))
                entities.Controller(slot).Kick(reason.c_str());
        });
        return;
    }
    auto* admin = AdminActions(_rt);
    if (!admin)
    {
        // Make the missing optional dependency visible.
        Log::Warn("[AC] cannot ban {}: admin-system is not loaded (no {}).", steamId,
                  Contracts::IAdminActions::InterfaceName);
        return;
    }

    if (const auto result = admin->Ban(steamId, _config.Get().anticheat.banDurationSec, reason);
        result != Contracts::BanResult::Ok)
        Log::Warn("[AC] ban for {} rejected by admin-system (code {}).", steamId, static_cast<int>(result));
}

}  // namespace Anticheat
