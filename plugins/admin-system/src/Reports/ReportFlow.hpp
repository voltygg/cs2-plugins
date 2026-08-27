#pragma once

#include "../Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <string>

namespace AdminSystem::Reports
{

/**
 * Everything a report accumulates while the reporter walks target -> reason -> confirm.
 * Copied by value through the menu callbacks.
 */
struct PendingReport
{
    /** Captured at selection and re-verified at every step, so slot reuse can't redirect the
     *  report; the name is resolved fresh at display time rather than carried here stale. */
    VoltMod::PlayerRef Target;
    std::string ReasonCode;
    std::string ReasonText;
};

/** Open the report player picker for @p reporterSlot - the `!report` entry point. */
void OpenReportMenu(AdminSystem::App& app, int reporterSlot);

}  // namespace AdminSystem::Reports
