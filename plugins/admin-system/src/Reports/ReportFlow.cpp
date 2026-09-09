#include "ReportFlow.hpp"

#include "../Admin/Menu/PlayerPicker.hpp"
#include "../Config/ReportSettings.hpp"
#include "../Core/App.hpp"
#include "ReportManager.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Strings.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/Flow.hpp>
#include <VoltMod/Messaging/Messages.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
        return "report.failed";

    if (!app.Runtime.Players.Get(pending.Target))
        return "report.targetLost";

    if (!app.Reports.CanReport(reporter->SteamId(), pending.Target.SteamId))
        return "report.blocked";

    return std::nullopt;
}

static void Submit(App& app, int reporterSlot, PendingReport& pending)
{
    auto* reporter = app.Runtime.Players.Get(reporterSlot);
    auto* target = app.Runtime.Players.Get(pending.Target);
    if (!reporter || !target)
        return;

    const int64_t reporterSteamId = reporter->SteamId();
    app.Reports.Submit(*reporter, *target, pending.ReasonCode, pending.ReasonText,
                       // The reporter may be gone by the time the write lands, and their old slot
                       // may host somebody else - re-find them by SteamID.
                       [&app, reporterSteamId, name = target->Name()](bool ok) {
                           auto* player = app.Runtime.Players.BySteamId(reporterSteamId);
                           if (!player)
                               return;
                           app.Runtime.Messages.ReplyKey(player->Slot(), ok ? "report.submitted" : "report.failed",
                                                         {{"name", name}});
                       });
}

static void StartReportFlow(App& app, int reporterSlot, VoltMod::PlayerRef targetRef)
{
    auto* target = app.Runtime.Players.Get(targetRef);
    if (!target)
        return;

    auto& tr = app.Runtime.Translations;

    // The flow runs for one reporter, so every step string resolves in their language here.
    std::vector<std::pair<std::string, std::string>> reasons;
    for (const auto& reason : app.Settings.GetReports().reasons)
        reasons.emplace_back(ReasonLabel(app, reason, reporterSlot), reason.code);

    ReportFlowT::Create(app.MenuFor(reporterSlot), reporterSlot, PendingReport{.Target = targetRef})
        ->Validate([&app, reporterSlot](const PendingReport& p) { return ValidatePending(app, reporterSlot, p); })
        ->AddOptionsStep({.Title = tr.Get("report.selectReason", reporterSlot),
                          .Options = std::move(reasons),
                          .Set =
                              [](PendingReport& p, const std::string& label, const std::string& code) {
                                  p.ReasonText = label;
                                  p.ReasonCode = code;
                              },
                          .CustomLabel = app.Settings.GetReports().allowCustomReason
                                             ? tr.Get("report.customReason", reporterSlot)
                                             : std::string(),
                          .CustomPrompt = tr.Get("report.customReasonPrompt", reporterSlot),
                          .CustomValue = CustomReasonCode})
        ->Confirm({.Title = tr.Get("report.confirmTitle", reporterSlot),
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
                   .ConfirmLabel = tr.Get("report.confirm", reporterSlot),
                   .CancelLabel = tr.Get("report.cancel", reporterSlot)})
        ->Finish([&app, reporterSlot](PendingReport& p) { Submit(app, reporterSlot, p); })
        ->Start();
}

void OpenReportMenu(AdminSystem::App& app, int reporterSlot)
{
    auto* reporter = app.Runtime.Players.Get(reporterSlot);
    if (!reporter)
        return;

    const int64_t reporterSteamId = reporter->SteamId();
    auto menu = Admin::Menu::BuildPlayerPicker(
        app, reporterSlot,
        {.Title = app.Runtime.Translations.Get("report.selectTarget", reporterSlot),
         .Pick = [&app, reporterSlot](VoltMod::PlayerRef target) { StartReportFlow(app, reporterSlot, target); },
         // The framework picker lists every connected player, so ineligible targets are greyed out here
         // rather than filtered out of the roster.
         .Enabled =
             [&app, reporterSlot, reporterSteamId](VoltMod::PlayerRef targetRef) {
                 if (targetRef.Slot == reporterSlot)
                     return false;
                 auto* target = app.Runtime.Players.Get(targetRef);
                 if (!target || target->IsBot())
                     return false;
                 return app.Reports.CanReport(reporterSteamId, target->SteamId());
             }});

    if (!menu)
        return;

    // Reporters may press !report mid-round, where being held still would get them killed. The
    // rest of the flow pushes onto this session, so it stays unfrozen throughout.
    app.OpenMenu(reporterSlot, std::move(menu), {.FreezeMovement = false});
}

}  // namespace AdminSystem::Reports
