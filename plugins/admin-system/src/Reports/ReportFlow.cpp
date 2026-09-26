#include "Reports/ReportFlow.hpp"

#include "App.hpp"
#include "Config/ReportSettings.hpp"
#include "Reports/ReportManager.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Labeled.hpp>
#include <VoltMod/Core/Text/Strings.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/Flow.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Messaging/Messages.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using VoltMod::Labeled;
using VoltMod::Strings;

namespace AdminSystem::Reports
{

using ReportFlowT = VoltMod::Flow<PendingReport>;

/** Reason code recorded when the reporter types their own text. */
// Owning: Flow::AddOptionsStep takes the custom value by value and keeps it in the step.
static const std::string CustomReasonCode = "other";

/** `report.reasons.<code>` when the translation files define it, else the config label - so
 *  operator-added codes need no translation entry. */
static std::string ReasonLabel(App& app, const Config::ReportReason& reason, int slot)
{
    return app.Runtime.Translations.GetOr("report.reasons." + reason.code, slot, reason.label);
}

/** Re-runs before every step and at confirm: the target may have left and the gate may have closed
 *  while the menu sat open. Flow renders these keys without token substitution, so keep them
 *  token-free. */
static std::optional<std::string> ValidatePending(App& app, int slot, const PendingReport& pending)
{
    auto* reporter = app.Runtime.Players.Get(slot);
    if (!reporter)
    {
        return "report.failed";
    }

    if (!app.Runtime.Players.Get(pending.Target))
    {
        return "report.targetLost";
    }

    if (!app.Reports.CanReport(reporter->SteamId(), pending.Target.SteamId))
    {
        return "report.blocked";
    }

    return std::nullopt;
}

static void Submit(App& app, int reporterSlot, PendingReport& pending)
{
    auto* reporter = app.Runtime.Players.Get(reporterSlot);
    auto* target = app.Runtime.Players.Get(pending.Target);
    if (!reporter || !target)
    {
        return;
    }

    const int64_t reporterSteamId = reporter->SteamId();
    app.Reports.Submit(*reporter, *target, pending.ReasonCode, pending.ReasonText,
                       // The reporter may be gone by the time the write lands, and their old slot
                       // may host somebody else - re-find them by SteamID.
                       [&app, reporterSteamId, name = target->Name()](bool ok) {
                           auto* player = app.Runtime.Players.BySteamId(reporterSteamId);
                           if (!player)
                           {
                               return;
                           }
                           app.Runtime.Messages.SendKey(player->Slot(), ok ? "report.submitted" : "report.failed",
                                                        {{"name", name}});
                       });
}

static void StartReportFlow(App& app, int reporterSlot, VoltMod::PlayerRef targetRef)
{
    auto* target = app.Runtime.Players.Get(targetRef);
    if (!target)
    {
        return;
    }

    auto& translations = app.Runtime.Translations;

    // The flow runs for one reporter, so every step string resolves in their language here.
    std::vector<Labeled<std::string>> reasons;
    for (const auto& reason : app.Settings.Get().reports.reasons)
    {
        reasons.push_back({.Label = ReasonLabel(app, reason, reporterSlot), .Value = reason.code});
    }

    ReportFlowT::Create(app.Runtime.Menus, reporterSlot, PendingReport{.Target = targetRef})
        ->Validate([&app, reporterSlot](const PendingReport& p) { return ValidatePending(app, reporterSlot, p); })
        ->AddOptionsStep({.Title = translations.Get("report.selectReason", reporterSlot),
                          .Options = std::move(reasons),
                          .Set =
                              [](PendingReport& p, const std::string& label, const std::string& code) {
                                  p.ReasonText = label;
                                  p.ReasonCode = code;
                              },
                          .CustomLabel = app.Settings.Get().reports.allowCustomReason
                                             ? translations.Get("report.customReason", reporterSlot)
                                             : std::string(),
                          .CustomPrompt = translations.Get("report.customReasonPrompt", reporterSlot),
                          .CustomValue = CustomReasonCode})
        ->Confirm({.Title = translations.Get("report.confirmTitle", reporterSlot),
                   .Summary =
                       [&app, reporterSlot](const PendingReport& pending, VoltMod::SummaryRows& rows) {
                           auto& translations = app.Runtime.Translations;
                           auto* targetPlayer = app.Runtime.Players.Get(pending.Target);
                           rows.Add(translations.Get("report.target", reporterSlot),
                                    targetPlayer ? targetPlayer->Name() : std::string())
                               .Add(translations.Get("report.reason", reporterSlot),
                                    Strings::TruncateUtf8(pending.ReasonText, 40));
                       },
                   // Its own wording rather than the framework's "Confirm".
                   .ConfirmLabel = translations.Get("report.confirm", reporterSlot),
                   .CancelLabel = translations.Get("report.cancel", reporterSlot)})
        ->Finish([&app, reporterSlot](PendingReport& p) { Submit(app, reporterSlot, p); })
        ->Begin();
}

void OpenReportMenu(AdminSystem::App& app, int reporterSlot)
{
    auto* reporter = app.Runtime.Players.Get(reporterSlot);
    if (!reporter)
    {
        return;
    }

    auto& translations = app.Runtime.Translations;
    VoltMod::MenuBuilder builder(translations.Get("report.selectTarget", reporterSlot));

    // Only reportable players are listed: a greyed-out row gives no reason when pressed.
    int listed = 0;
    for (auto* target : app.Runtime.Players.All())
    {
        if (target->Slot() == reporterSlot || target->IsBot() ||
            !app.Reports.CanReport(reporter->SteamId(), target->SteamId()))
        {
            continue;
        }

        builder.Add(VoltMod::ButtonRow{
            .Label = target->Name(),
            .Activate = [&app, reporterSlot, ref = target->Ref()](int) { StartReportFlow(app, reporterSlot, ref); }});
        ++listed;
    }

    if (listed == 0)
    {
        builder.Text(translations.Get("common.noPlayers", reporterSlot));
    }

    app.Runtime.Menus.OpenSession(reporterSlot, builder.Build(), {});
}

}  // namespace AdminSystem::Reports
