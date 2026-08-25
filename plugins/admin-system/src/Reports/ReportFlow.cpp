#include "ReportFlow.hpp"

#include "../Admin/Menu/PlayerPicker.hpp"
#include "../Core/App.hpp"
#include "../Core/Config.hpp"
#include "ReportManager.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/StringUtils.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/Flow.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <VoltMod/Sdk/Messaging/UserMessage.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using VoltMod::Core::StringUtils;

namespace AdminSystem::Reports
{

using ReportFlowT = VoltMod::Flow<PendingReport>;

namespace
{

/** Reason code recorded when the reporter types their own text. */
constexpr const char* CustomReasonCode = "other";

/** `report.reasons.<code>` when the translation files define it, else the config label - so
 *  operator-added codes need no translation entry. Get() echoes a missing key back verbatim. */
std::string ReasonLabel(App& app, const Core::ReportReason& reason, int slot)
{
    const std::string key = "report.reasons." + reason.code;
    std::string text = app.Runtime.Translations.Get(key, slot);
    return text == key ? reason.label : text;
}

/** Re-runs before every step and at confirm: the target may have left and the gate may have closed
 *  while the menu sat open. Flow renders these keys without token substitution, so keep them
 *  token-free. */
std::optional<std::string> ValidatePending(App& app, int slot, const PendingReport& pending)
{
    auto* reporter = app.Runtime.Players.GetPlayerBySlot(slot);
    if (!reporter)
        return "report.failed";

    if (!app.Runtime.Players.GetPlayerBySlotIfSteamId(pending.TargetSlot, pending.TargetSteamId))
        return "report.targetLost";

    if (!app.Reports.CanReport(reporter->GetSteamID(), pending.TargetSteamId))
        return "report.blocked";

    return std::nullopt;
}

void Submit(App& app, int reporterSlot, PendingReport& pending)
{
    auto* reporter = app.Runtime.Players.GetPlayerBySlot(reporterSlot);
    auto* target = app.Runtime.Players.GetPlayerBySlot(pending.TargetSlot);
    if (!reporter || !target)
        return;

    const int64_t reporterSteamId = reporter->GetSteamID();
    app.Reports.Submit(*reporter, *target, pending.ReasonCode, pending.ReasonText,
                       // The reporter may be gone by the time the write lands, and their old slot
                       // may host somebody else - re-find them by SteamID.
                       [&app, reporterSteamId, name = pending.TargetName](bool ok) {
                           auto* player = app.Runtime.Players.GetPlayerBySteamId(reporterSteamId);
                           if (!player)
                               return;
                           app.Runtime.Messages.ReplyKey(player->GetSlot(), ok ? "report.submitted" : "report.failed",
                                                         {{"name", name}});
                       });
}

void StartReportFlow(App& app, int reporterSlot, int targetSlot)
{
    auto* target = app.Runtime.Players.GetPlayerBySlot(targetSlot);
    if (!target)
        return;

    ReportFlowT::Create(
        app.Runtime.Menus,
        PendingReport{.TargetSlot = targetSlot, .TargetSteamId = target->GetSteamID(), .TargetName = target->GetName()})
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
        ->WithConfirm([&app](int slot) { return app.Runtime.Translations.Get("report.confirmTitle", slot); },
                      [&app](int slot, const PendingReport& pending) {
                          auto& tr = app.Runtime.Translations;
                          std::vector<std::pair<std::string, std::string>> rows;
                          rows.emplace_back(tr.Get("report.target", slot), pending.TargetName);
                          rows.emplace_back(tr.Get("report.reason", slot),
                                            StringUtils::TruncateUtf8(pending.ReasonText, 40));
                          return rows;
                      },
                      [&app](int slot) { return app.Runtime.Translations.Get("report.confirm", slot); },
                      [&app](int slot) { return app.Runtime.Translations.Get("report.cancel", slot); })
        ->OnFinish([&app](int slot, PendingReport& p) { Submit(app, slot, p); })
        ->Start(reporterSlot);
}

}  // namespace

void OpenReportMenu(AdminSystem::App& app, int reporterSlot)
{
    auto* reporter = app.Runtime.Players.GetPlayerBySlot(reporterSlot);
    if (!reporter)
        return;

    const int64_t reporterSteamId = reporter->GetSteamID();
    auto menu = Admin::Menu::BuildPlayerPicker(
        app, reporterSlot, app.Runtime.Translations.Get("report.selectTarget", reporterSlot),
        [&app](int slot, int targetSlot) { StartReportFlow(app, slot, targetSlot); },
        // The framework picker lists every connected player, so ineligible targets are greyed out here
        // rather than filtered out of the roster.
        [&app, reporterSlot, reporterSteamId](int targetSlot) {
            if (targetSlot == reporterSlot)
                return false;
            auto* target = app.Runtime.Players.GetPlayerBySlot(targetSlot);
            if (!target || target->IsBot())
                return false;
            return app.Reports.CanReport(reporterSteamId, target->GetSteamID());
        });

    if (!menu)
        return;

    // Reporters may press !report mid-round, where being held still would get them killed. The
    // rest of the flow pushes onto this session, so it stays unfrozen throughout.
    app.Runtime.Menus.OpenMenu(reporterSlot, menu, {.FreezeMovement = false});
}

}  // namespace AdminSystem::Reports
