#pragma once

#include <cstdint>
#include <string>

namespace AdminSystem::Reports
{

/**
 * Everything a report accumulates while the reporter walks target -> reason -> confirm.
 * Copied by value through the menu callbacks.
 */
struct PendingReport
{
    int TargetSlot = -1;
    /** Captured at selection and re-verified at every step, so slot reuse can't redirect the report. */
    int64_t TargetSteamId = 0;
    std::string TargetName;
    std::string ReasonCode;
    std::string ReasonText;
};

/** Open the report player picker for @p reporterSlot - the `!report` entry point. */
void OpenReportMenu(int reporterSlot);

}  // namespace AdminSystem::Reports
