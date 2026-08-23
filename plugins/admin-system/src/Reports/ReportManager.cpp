#include "ReportManager.hpp"

#include "../Core/App.hpp"
#include "../Database/Repositories/ReportRepository.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Log.hpp>
#include <CS2Kit/Core/TimeUtils.hpp>
#include <CS2Kit/Runtime.hpp>
#include <algorithm>
#include <utility>

using CS2Kit::Core::TimeUtils;

namespace AdminSystem::Reports
{

namespace Log = CS2Kit::Core::Log;

ReportGate ReportManager::EvaluateGate(int64_t reporterSteamId, std::optional<int64_t> targetSteamId, int64_t now) const
{
    const auto& config = _app.Config.GetReports();
    if (!config.enabled)
        return {ReportDenial::Disabled};

    if (int64_t wait = _anyTarget.RemainingSec(reporterSteamId, now, config.cooldownSec); wait > 0)
        return {ReportDenial::OnCooldown, wait};

    if (targetSteamId)
    {
        const auto pair = std::pair{reporterSteamId, *targetSteamId};
        if (int64_t wait = _perTarget.RemainingSec(pair, now, config.duplicateWindowSec); wait > 0)
            return {ReportDenial::OnCooldown, wait};
    }

    return {};
}

ReportGate ReportManager::CanReport(int64_t reporterSteamId) const
{
    return EvaluateGate(reporterSteamId, std::nullopt, TimeUtils::Now());
}

bool ReportManager::CanReport(int64_t reporterSteamId, int64_t targetSteamId) const
{
    return static_cast<bool>(EvaluateGate(reporterSteamId, targetSteamId, TimeUtils::Now()));
}

void ReportManager::Submit(const CS2Kit::Player& reporter, const CS2Kit::Player& target, const std::string& reasonCode,
                           const std::string& reasonText, std::function<void(bool)> onDone)
{
    const int64_t reporterSteamId = reporter.GetSteamID();
    const int64_t targetSteamId = target.GetSteamID();
    const int64_t now = TimeUtils::Now();

    if (!EvaluateGate(reporterSteamId, targetSteamId, now))
    {
        if (onDone)
            onDone(false);
        return;
    }

    Database::Report report{
        .ReporterSteamId = reporterSteamId,
        .ReporterName = reporter.GetName(),
        .ReporterIp = reporter.GetIpAddress(),
        .TargetSteamId = targetSteamId,
        .TargetName = target.GetName(),
        .TargetIp = target.GetIpAddress(),
        .ReasonCode = reasonCode,
        .Reason = reasonText,
        .ServerTag = _app.Config.GetServer().tag,
        .MapName = _app.Runtime.CurrentMap,
        .CreatedAt = now,
    };

    Arm(reporterSteamId, targetSteamId, now);
    Log::Info("Report: {} ({}) reported {} ({}) for '{}' [{}]", report.ReporterName, reporterSteamId, report.TargetName,
              targetSteamId, reasonText, reasonCode);

    Database::ReportRepository{_app.Db}.CreateAsync(
        report, [this, reporterSteamId, targetSteamId, onDone = std::move(onDone)](bool ok) {
            // The write is the only database-health signal there is, so a failure refunds the
            // attempt rather than costing the reporter a cooldown.
            if (!ok)
                Release(reporterSteamId, targetSteamId);
            if (onDone)
                onDone(ok);
        });
}

void ReportManager::Arm(int64_t reporterSteamId, int64_t targetSteamId, int64_t now)
{
    _anyTarget.Acquire(reporterSteamId, now);
    _perTarget.Acquire({reporterSteamId, targetSteamId}, now);

    // Both maps only grow here, so this is the one place worth sweeping.
    const auto& config = _app.Config.GetReports();
    const int64_t horizon = std::max(config.cooldownSec, config.duplicateWindowSec);
    _anyTarget.Prune(now, horizon);
    _perTarget.Prune(now, horizon);
}

void ReportManager::Release(int64_t reporterSteamId, int64_t targetSteamId)
{
    _anyTarget.Reset(reporterSteamId);
    _perTarget.Reset({reporterSteamId, targetSteamId});
}

}  // namespace AdminSystem::Reports
