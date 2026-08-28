#include "ReportFlow.hpp"

#include "../Admin/Menu/PlayerPicker.hpp"
#include "../Core/App.hpp"
#include "../Core/Config.hpp"
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
 *  operator-added codes need no translation entry. Get() echoes a missing key back verbatim. */
static std::string ReasonLabel(App& app, const Core::ReportReason& reason, int slot)
{
    const std::string key = "report.reasons." + reason.code;
    std::string text = app.Runtime.Translations.Get(key, slot);
    return text == key ? reason.label : text;
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

static void StartReportFlow(App& app, int reporterSlot, int targetSlot)
{
    auto* target = app.Runtime.Players.Get(targetSlot);
    if (!target)
        return;

    ReportFlowT::Create(app.Runtime.HtmlMenus, PendingReport{.Target = target->Ref()})
        ->OnValidate([&app](int slot, const PendingReport& p) { return ValidatePending(app, slot, p); })
        ->AddOptionsStep([&app](int slot) { return app.Runtime.Translations.Get("report.selectReason", slot); },
                         [&app](int slot) {
                             std::vector<ReportFlowT::Option> reasons;
                             for (const auto& reason : app.Config.GetReports().reasons)
                                 reasons.emplace_back(ReasonLabel(app, reason, slot), reason.code);
                             return reasons;
                         },
                         [](PendingReport& p, const std::string& label, const std::string& code) {
                             p.ReasonText = label;
                             p.ReasonCode = code;
                         },
                         [&app](int slot) {
                             return app.Config.GetReports().allowCustomReason
                                        ? app.Runtime.Translations.Get("report.customReason", slot)
                                        : std::string();
                         },
                         [&app](int slot) { return app.Runtime.Translations.Get("report.customReasonPrompt", slot); },
                         CustomReasonCode)
        ->WithConfirm(
            [&app](int slot) { return app.Runtime.Translations.Get("report.confirmTitle", slot); },
            [&app](int slot, const PendingReport& pending) {
                auto& tr = app.Runtime.Translations;
                std::vector<std::pair<std::string, std::string>> rows;
                auto* targetPlayer = app.Runtime.Players.Get(pending.Target);
                rows.emplace_back(tr.Get("report.target", slot), targetPlayer ? targetPlayer->Name() : std::string());
                rows.emplace_back(tr.Get("report.reason", slot), Strings::TruncateUtf8(pending.ReasonText, 40));
                return rows;
            },
            [&app](int slot) { return app.Runtime.Translations.Get("report.confirm", slot); },
            [&app](int slot) { return app.Runtime.Translations.Get("report.cancel", slot); })
        ->OnFinish([&app](int slot, PendingReport& p) { Submit(app, slot, p); })
        ->Start(reporterSlot);
}

void OpenReportMenu(AdminSystem::App& app, int reporterSlot)
{
    auto* reporter = app.Runtime.Players.Get(reporterSlot);
    if (!reporter)
        return;

    const int64_t reporterSteamId = reporter->SteamId();
    auto menu = Admin::Menu::BuildPlayerPicker(
        app, reporterSlot, app.Runtime.Translations.Get("report.selectTarget", reporterSlot),
        [&app](int slot, int targetSlot) { StartReportFlow(app, slot, targetSlot); },
        // The framework picker lists every connected player, so ineligible targets are greyed out here
        // rather than filtered out of the roster.
        [&app, reporterSlot, reporterSteamId](int targetSlot) {
            if (targetSlot == reporterSlot)
                return false;
            auto* target = app.Runtime.Players.Get(targetSlot);
            if (!target || target->IsBot())
                return false;
            return app.Reports.CanReport(reporterSteamId, target->SteamId());
        });

    if (!menu)
        return;

    // Reporters may press !report mid-round, where being held still would get them killed. The
    // rest of the flow pushes onto this session, so it stays unfrozen throughout.
    app.Runtime.HtmlMenus.OpenMenu(reporterSlot, menu, {.FreezeMovement = false});
}

}  // namespace AdminSystem::Reports
