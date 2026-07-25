#include "ReportManager.hpp"

#include "../Core/Managers.hpp"
#include "../Database/Repositories/ReportRepository.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/TimeUtils.hpp>
#include <algorithm>
#include <utility>

using CS2Kit::Core::Engine;
using CS2Kit::Utils::TimeUtils;

namespace AdminSystem::Reports
{

namespace Log = CS2Kit::Utils::Log;

ReportGate ReportManager::EvaluateGate(int64_t reporterSteamId, std::optional<int64_t> targetSteamId, int64_t now) const
{
    const auto& config = App().Config.GetReports();
    if (!config.enabled)
        return {ReportDenial::Disabled};

    // The elapsed >= 0 checks below absorb a backwards clock jump, which would otherwise read as
    // "inside every window".
    if (config.cooldownSec > 0)
    {
        if (auto it = _lastReportAt.find(reporterSteamId); it != _lastReportAt.end())
        {
            const int64_t elapsed = now - it->second;
            if (elapsed >= 0 && elapsed < config.cooldownSec)
                return {ReportDenial::OnCooldown, config.cooldownSec - elapsed};
        }
    }

    if (targetSteamId && config.duplicateWindowSec > 0)
    {
        if (auto it = _lastReportOfPair.find({reporterSteamId, *targetSteamId}); it != _lastReportOfPair.end())
        {
            const int64_t elapsed = now - it->second;
            if (elapsed >= 0 && elapsed < config.duplicateWindowSec)
                return {ReportDenial::OnCooldown, config.duplicateWindowSec - elapsed};
        }
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
        .ServerTag = App().Config.GetServer().tag,
        .MapName = Engine().CurrentMap,
        .CreatedAt = now,
    };

    Arm(reporterSteamId, targetSteamId, now);
    Log::Info("Report: {} ({}) reported {} ({}) for '{}' [{}]", report.ReporterName, reporterSteamId, report.TargetName,
              targetSteamId, reasonText, reasonCode);

    Database::ReportRepository{}.CreateAsync(
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
    _lastReportAt[reporterSteamId] = now;
    _lastReportOfPair[{reporterSteamId, targetSteamId}] = now;
    Prune(now);
}

void ReportManager::Release(int64_t reporterSteamId, int64_t targetSteamId)
{
    _lastReportAt.erase(reporterSteamId);
    _lastReportOfPair.erase({reporterSteamId, targetSteamId});
}

void ReportManager::Prune(int64_t now)
{
    const auto& config = App().Config.GetReports();
    const int64_t horizon = std::max(config.cooldownSec, config.duplicateWindowSec);

    std::erase_if(_lastReportAt, [&](const auto& entry) { return now - entry.second > horizon; });
    std::erase_if(_lastReportOfPair, [&](const auto& entry) { return now - entry.second > horizon; });
}

}  // namespace AdminSystem::Reports
