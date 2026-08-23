#include "ReportFlow.hpp"

#include "../Admin/Menu/PlayerPicker.hpp"
#include "../Core/Config.hpp"
#include "../Core/Managers.hpp"
#include "ReportManager.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/App/Services.hpp>
#include <CS2Kit/Menu/Flow.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/UserMessage.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using CS2Kit::App::Engine;
using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Utils::StringUtils;

namespace AdminSystem::Reports
{

using ReportFlowT = CS2Kit::Flow<PendingReport>;

namespace
{

/** Reason code recorded when the reporter types their own text. */
constexpr const char* CustomReasonCode = "other";

/** `report.reasons.<code>` when the translation files define it, else the config label - so
 *  operator-added codes need no translation entry. Get() echoes a missing key back verbatim. */
std::string ReasonLabel(const Core::ReportReason& reason, int slot)
{
    const std::string key = "report.reasons." + reason.code;
    std::string text = Engine().Utils.Translations.Get(key, slot);
    return text == key ? reason.label : text;
}

/** Re-runs before every step and at confirm: the target may have left and the gate may have closed
 *  while the menu sat open. Flow renders these keys without token substitution, so keep them
 *  token-free. */
std::optional<std::string> ValidatePending(int slot, const PendingReport& pending)
{
    auto* reporter = Engine().Players.GetPlayerBySlot(slot);
    if (!reporter)
        return "report.failed";

    if (!Engine().Players.GetPlayerBySlotIfSteamId(pending.TargetSlot, pending.TargetSteamId))
        return "report.targetLost";

    if (!App().Reports.CanReport(reporter->GetSteamID(), pending.TargetSteamId))
        return "report.blocked";

    return std::nullopt;
}

/** Hand-built rather than Flow::AddOptionsStep so each row carries its stable `code` alongside the
 *  localized label, instead of recovering one from the other. */
std::shared_ptr<CS2Kit::MenuView> BuildReasonStep(int slot, ReportFlowT& flow)
{
    auto self = flow.shared_from_this();
    auto& tr = Engine().Utils.Translations;
    const auto& config = App().Config.GetReports();

    MenuBuilder builder(tr.Get("report.selectReason", slot));
    for (const auto& reason : config.reasons)
    {
        std::string label = ReasonLabel(reason, slot);
        builder.AddButton(label, [self, code = reason.code, label](int pickedBy) {
            self->State().ReasonCode = code;
            self->State().ReasonText = label;
            self->Advance(pickedBy);
        });
    }

    if (config.allowCustomReason)
    {
        builder.AddInput(
            tr.Get("report.customReason", slot), tr.Get("report.customReasonPrompt", slot),
            [](int) { return std::string(); },
            [self](int typedBy, std::string_view text) {
                std::string value = StringUtils::Trim(std::string(text));
                if (value.empty())
                    return false;  // re-prompt
                self->State().ReasonCode = CustomReasonCode;
                self->State().ReasonText = std::move(value);
                self->Advance(typedBy);
                return true;
            });
    }

    return builder.Build();
}

void Submit(int reporterSlot, PendingReport& pending)
{
    auto* reporter = Engine().Players.GetPlayerBySlot(reporterSlot);
    auto* target = Engine().Players.GetPlayerBySlot(pending.TargetSlot);
    if (!reporter || !target)
        return;

    const int64_t reporterSteamId = reporter->GetSteamID();
    App().Reports.Submit(*reporter, *target, pending.ReasonCode, pending.ReasonText,
                         // The reporter may be gone by the time the write lands, and their old slot
                         // may host somebody else - re-find them by SteamID.
                         [reporterSteamId, name = pending.TargetName](bool ok) {
                             auto* player = Engine().Players.GetPlayerBySteamId(reporterSteamId);
                             if (!player)
                                 return;
                             Engine().Sdk.Messages.ReplyKey(player->GetSlot(), ok ? "report.submitted" : "report.failed",
                                                        {{"name", name}});
                         });
}

void StartReportFlow(int reporterSlot, int targetSlot)
{
    auto* target = Engine().Players.GetPlayerBySlot(targetSlot);
    if (!target)
        return;

    ReportFlowT::Create(
        PendingReport{.TargetSlot = targetSlot, .TargetSteamId = target->GetSteamID(), .TargetName = target->GetName()})
        ->OnValidate(ValidatePending)
        ->AddStep(BuildReasonStep)
        ->WithConfirm([](int slot) { return Engine().Utils.Translations.Get("report.confirmTitle", slot); },
                      [](int slot, const PendingReport& pending) {
                          auto& tr = Engine().Utils.Translations;
                          std::vector<std::pair<std::string, std::string>> rows;
                          rows.emplace_back(tr.Get("report.target", slot), pending.TargetName);
                          rows.emplace_back(tr.Get("report.reason", slot),
                                            StringUtils::TruncateUtf8(pending.ReasonText, 40));
                          return rows;
                      },
                      [](int slot) { return Engine().Utils.Translations.Get("report.confirm", slot); },
                      [](int slot) { return Engine().Utils.Translations.Get("report.cancel", slot); })
        ->OnFinish(Submit)
        ->Start(reporterSlot);
}

}  // namespace

void OpenReportMenu(int reporterSlot)
{
    auto* reporter = Engine().Players.GetPlayerBySlot(reporterSlot);
    if (!reporter)
        return;

    const int64_t reporterSteamId = reporter->GetSteamID();
    auto menu = Admin::Menu::BuildPlayerPicker(
        reporterSlot, Engine().Utils.Translations.Get("report.selectTarget", reporterSlot),
        [](int slot, int targetSlot) { StartReportFlow(slot, targetSlot); },
        // The kit picker lists every connected player, so ineligible targets are greyed out here
        // rather than filtered out of the roster.
        [reporterSlot, reporterSteamId](int targetSlot) {
            if (targetSlot == reporterSlot)
                return false;
            auto* target = Engine().Players.GetPlayerBySlot(targetSlot);
            if (!target || target->IsBot())
                return false;
            return App().Reports.CanReport(reporterSteamId, target->GetSteamID());
        });

    if (!menu)
        return;

    // Reporters may press !report mid-round, where being held still would get them killed. The
    // rest of the flow pushes onto this session, so it stays unfrozen throughout.
    Engine().Menus.OpenMenu(reporterSlot, menu, {.FreezeMovement = false});
}

}  // namespace AdminSystem::Reports
