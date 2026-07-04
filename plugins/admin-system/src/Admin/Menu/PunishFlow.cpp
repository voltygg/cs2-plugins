#include "PunishFlow.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Config.hpp"
#include "../../Core/Managers.hpp"
#include "../../Punishments/IssuePunishment.hpp"
#include "../AdminManager.hpp"
#include "Labels.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/Flow.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Menu
{

using namespace AdminSystem::Punishments;

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Utils::StringUtils;
using PunishFlowT = CS2Kit::Flow<PendingPunishment>;

namespace
{

/** True if @p adminSlot may still punish @p targetSlot with @p type's permission. */
bool CanStillPunish(int adminSlot, int targetSlot, PunishType type)
{
    auto& plrMgr = Engine().Players;
    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!admin || !target)
        return false;
    return App().Admins.CanActOn(admin->GetSteamID(), target->GetSteamID(), PermissionFor(type));
}

/** Flow validation: the target may have left (or the slot rehosts another player) and the
 *  admin's flags/immunity may have changed (e.g. !admin_reload) while the menu was open. */
std::optional<std::string> ValidatePending(int slot, const PendingPunishment& pending)
{
    auto* target = Engine().Players.GetPlayerBySlot(pending.TargetSlot);
    if (!target || target->GetSteamID() != pending.TargetSteamId)
        return "punish.targetLost";
    if (!CanStillPunish(slot, pending.TargetSlot, pending.Type))
        return "punish.notAllowed";
    return std::nullopt;
}

void Issue(int adminSlot, PendingPunishment& pending)
{
    auto& tr = Engine().Translations;
    auto* admin = Engine().Players.GetPlayerBySlot(adminSlot);
    auto* target = Engine().Players.GetPlayerBySlot(pending.TargetSlot);
    if (!admin || !target)
        return;

    if (!IssuePunishment(*admin, *target, pending.Type, pending.Reason, pending.DurationSec))
    {
        App().Chat.Reply(adminSlot, tr.Get("punish.failed", adminSlot,
                                           {{"action", tr.Get(ActionTranslationKey(pending.Type), adminSlot)}}));
    }
    else if (!App().Config.GetChat().broadcastPunishments)
    {
        // With broadcasts on, the admin already sees the server-wide line; avoid double messaging.
        App().Chat.Reply(adminSlot, tr.Get("punish.issued", adminSlot,
                                           {{"action", tr.Get(ActionTranslationKey(pending.Type), adminSlot)},
                                            {"name", pending.TargetName}}));
    }
}

/** The validated confirm -> issue tail every punish path shares. */
PunishFlowT::Ptr MakeBaseFlow(PendingPunishment pending)
{
    auto type = pending.Type;
    return PunishFlowT::Create(std::move(pending))
        ->OnValidate(ValidatePending)
        ->WithConfirm(
            [type](int slot) {
                auto& tr = Engine().Translations;
                return std::format("{}: {}", tr.Get("punish.confirmTitle", slot),
                                   tr.Get(ActionTranslationKey(type), slot));
            },
            [](int slot, const PendingPunishment& p) {
                auto& tr = Engine().Translations;
                std::vector<std::pair<std::string, std::string>> rows;
                rows.emplace_back(tr.Get("punish.target", slot), p.TargetName);
                if (IsTimed(p.Type))
                    rows.emplace_back(tr.Get("punish.duration", slot), DurationLabel(p.DurationSec, slot));
                rows.emplace_back(tr.Get("punish.reason", slot), StringUtils::TruncateUtf8(p.Reason, 40));
                return rows;
            },
            [](int slot) { return Engine().Translations.Get("punish.confirm", slot); },
            [](int slot) { return Engine().Translations.Get("punish.cancel", slot); })
        ->OnFinish(Issue);
}

}  // namespace

void StartPunishFlow(int adminSlot, PendingPunishment pending)
{
    auto type = pending.Type;
    auto stepTitle = [type](int slot, const char* suffixKey) {
        auto& tr = Engine().Translations;
        return std::format("{}: {}", tr.Get(ActionTranslationKey(type), slot), tr.Get(suffixKey, slot));
    };

    MakeBaseFlow(std::move(pending))
        ->AddDurationStep(
            [stepTitle](int slot) { return stepTitle(slot, "panel.selectDuration"); },
            [](int slot) {
                std::vector<std::pair<std::string, int>> presets;
                for (int seconds : App().Config.GetMenuDurations())
                    presets.emplace_back(DurationLabel(seconds, slot), seconds);
                return presets;
            },
            [](PendingPunishment& p, int seconds) { p.DurationSec = seconds; },
            [](int slot) { return Engine().Translations.Get("duration.custom", slot); },
            [](int slot) { return Engine().Translations.Get("duration.customPrompt", slot); },
            [](const PendingPunishment& p) { return IsTimed(p.Type); })
        ->AddOptionsStep(
            [stepTitle](int slot) { return stepTitle(slot, "punish.selectReason"); },
            [](int) { return App().Config.GetPunishments().reasonPresets; },
            [](PendingPunishment& p, std::string reason) { p.Reason = std::move(reason); },
            [](int slot) { return Engine().Translations.Get("punish.customReason", slot); },
            [](int slot) { return Engine().Translations.Get("punish.customReasonPrompt", slot); })
        ->Start(adminSlot);
}

bool AnyTemplateUsable(int adminSlot, int targetSlot)
{
    for (const auto& tmpl : App().Config.GetPunishmentTemplates())
    {
        if (CanStillPunish(adminSlot, targetSlot, tmpl.Type))
            return true;
    }
    return false;
}

std::shared_ptr<CS2Kit::MenuView> BuildQuickPunishMenu(int adminSlot, int targetSlot)
{
    auto& tr = Engine().Translations;
    auto* target = Engine().Players.GetPlayerBySlot(targetSlot);
    if (!target)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", tr.Get("punish.quickPunish", adminSlot), target->GetName()));

    int rows = 0;
    for (const auto& tmpl : App().Config.GetPunishmentTemplates())
    {
        if (!CanStillPunish(adminSlot, targetSlot, tmpl.Type))
            continue;

        PendingPunishment pending{
            .Type = tmpl.Type,
            .TargetSlot = targetSlot,
            .TargetSteamId = target->GetSteamID(),
            .TargetName = target->GetName(),
            .DurationSec = tmpl.DurationSec,
            .Reason = tmpl.Reason,
        };
        // Duration and reason are preset by the template, so the flow jumps straight to confirm.
        builder.AddButton(std::format("{} - {}", tmpl.Name, DurationLabel(tmpl.DurationSec, adminSlot)),
                          [pending](int slot) { MakeBaseFlow(pending)->Start(slot); });
        ++rows;
    }

    // Permissions can change between the actions menu and here; never show a dead-end empty page.
    if (rows == 0)
        builder.AddButton(tr.Get("punish.noTemplates", adminSlot), [](int) {}, false);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
