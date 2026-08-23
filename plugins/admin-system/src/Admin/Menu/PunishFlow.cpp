#include "PunishFlow.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Core/Config.hpp"
#include "../../Punishments/IssuePunishment.hpp"
#include "../AdminManager.hpp"
#include "Labels.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/StringUtils.hpp>
#include <CS2Kit/Core/Translations.hpp>
#include <CS2Kit/Menu/Flow.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Runtime.hpp>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace AdminSystem::Admin::Menu
{

using namespace AdminSystem::Punishments;

using CS2Kit::Core::StringUtils;
using CS2Kit::Menu::MenuBuilder;
using PunishFlowT = CS2Kit::Flow<PendingPunishment>;

namespace
{

/** True if @p adminSlot may still punish @p targetSlot with @p type's permission. */
bool CanStillPunish(App& app, int adminSlot, int targetSlot, PunishType type)
{
    auto& plrMgr = app.Runtime.Players;
    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!admin || !target)
        return false;
    return app.Access.CanActOn(admin->GetSteamID(), target->GetSteamID(), PermissionFor(type));
}

/** Flow validation: the target may have left (or the slot rehosts another player) and the
 *  admin's flags/immunity may have changed (e.g. !admin_reload) while the menu was open. */
std::optional<std::string> ValidatePending(App& app, int slot, const PendingPunishment& pending)
{
    if (!app.Runtime.Players.GetPlayerBySlotIfSteamId(pending.TargetSlot, pending.TargetSteamId))
        return "punish.targetLost";
    if (!CanStillPunish(app, slot, pending.TargetSlot, pending.Type))
        return "punish.notAllowed";
    return std::nullopt;
}

void Issue(App& app, int adminSlot, PendingPunishment& pending)
{
    auto& tr = app.Runtime.Translations;
    auto* admin = app.Runtime.Players.GetPlayerBySlot(adminSlot);
    auto* target = app.Runtime.Players.GetPlayerBySlot(pending.TargetSlot);
    if (!admin || !target)
        return;

    if (!IssuePunishment(app, *admin, *target, pending.Type, pending.Reason, pending.DurationSec))
    {
        app.Chat.Reply(adminSlot, tr.Get("punish.failed", adminSlot,
                                         {{"action", tr.Get(ActionTranslationKey(pending.Type), adminSlot)}}));
    }
    else if (!app.Config.GetChat().broadcastPunishments)
    {
        // With broadcasts on, the admin already sees the server-wide line; avoid double messaging.
        app.Chat.Reply(adminSlot, tr.Get("punish.issued", adminSlot,
                                         {{"action", tr.Get(ActionTranslationKey(pending.Type), adminSlot)},
                                          {"name", pending.TargetName}}));
    }
}

/** The validated confirm -> issue tail every punish path shares. */
PunishFlowT::Ptr MakeBaseFlow(App& app, PendingPunishment pending)
{
    auto type = pending.Type;
    return PunishFlowT::Create(std::move(pending))
        ->OnValidate([&app](int slot, const PendingPunishment& p) { return ValidatePending(app, slot, p); })
        ->WithConfirm(
            [&app, type](int slot) {
                auto& tr = app.Runtime.Translations;
                return std::format("{}: {}", tr.Get("punish.confirmTitle", slot),
                                   tr.Get(ActionTranslationKey(type), slot));
            },
            [&app](int slot, const PendingPunishment& p) {
                auto& tr = app.Runtime.Translations;
                std::vector<std::pair<std::string, std::string>> rows;
                rows.emplace_back(tr.Get("punish.target", slot), p.TargetName);
                if (IsTimed(p.Type))
                    rows.emplace_back(tr.Get("punish.duration", slot), DurationLabel(tr, p.DurationSec, slot));
                rows.emplace_back(tr.Get("punish.reason", slot), StringUtils::TruncateUtf8(p.Reason, 40));
                return rows;
            },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.confirm", slot); },
            [&app](int slot) { return app.Runtime.Translations.Get("punish.cancel", slot); })
        ->OnFinish([&app](int adminSlot, PendingPunishment& p) { Issue(app, adminSlot, p); });
}

}  // namespace

void StartPunishFlow(AdminSystem::App& app, int adminSlot, PendingPunishment pending)
{
    auto type = pending.Type;
    auto stepTitle = [&app, type](int slot, const char* suffixKey) {
        auto& tr = app.Runtime.Translations;
        return std::format("{}: {}", tr.Get(ActionTranslationKey(type), slot), tr.Get(suffixKey, slot));
    };

    MakeBaseFlow(app, std::move(pending))
        ->AddDurationStep([stepTitle](int slot) { return stepTitle(slot, "panel.selectDuration"); },
                          [&app](int slot) {
                              auto& tr = app.Runtime.Translations;
                              std::vector<std::pair<std::string, int>> presets;
                              for (int seconds : app.Config.GetMenuDurations())
                                  presets.emplace_back(DurationLabel(tr, seconds, slot), seconds);
                              return presets;
                          },
                          [](PendingPunishment& p, int seconds) { p.DurationSec = seconds; },
                          [&app](int slot) { return app.Runtime.Translations.Get("duration.custom", slot); },
                          [&app](int slot) { return app.Runtime.Translations.Get("duration.customPrompt", slot); },
                          [](const PendingPunishment& p) { return IsTimed(p.Type); })
        ->AddOptionsStep([stepTitle](int slot) { return stepTitle(slot, "punish.selectReason"); },
                         [&app](int) { return app.Config.GetPunishments().reasonPresets; },
                         [](PendingPunishment& p, std::string reason) { p.Reason = std::move(reason); },
                         [&app](int slot) { return app.Runtime.Translations.Get("punish.customReason", slot); },
                         [&app](int slot) { return app.Runtime.Translations.Get("punish.customReasonPrompt", slot); })
        ->Start(adminSlot);
}

bool AnyTemplateUsable(AdminSystem::App& app, int adminSlot, int targetSlot)
{
    for (const auto& tmpl : app.Config.GetPunishmentTemplates())
    {
        if (CanStillPunish(app, adminSlot, targetSlot, tmpl.Type))
            return true;
    }
    return false;
}

std::shared_ptr<CS2Kit::MenuView> BuildQuickPunishMenu(AdminSystem::App& app, int adminSlot, int targetSlot)
{
    auto& tr = app.Runtime.Translations;
    auto* target = app.Runtime.Players.GetPlayerBySlot(targetSlot);
    if (!target)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", tr.Get("punish.quickPunish", adminSlot), target->GetName()));

    int rows = 0;
    for (const auto& tmpl : app.Config.GetPunishmentTemplates())
    {
        if (!CanStillPunish(app, adminSlot, targetSlot, tmpl.Type))
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
        builder.AddButton(std::format("{} - {}", tmpl.Name, DurationLabel(tr, tmpl.DurationSec, adminSlot)),
                          [&app, pending](int slot) { MakeBaseFlow(app, pending)->Start(slot); });
        ++rows;
    }

    // Permissions can change between the actions menu and here; never show a dead-end empty page.
    if (rows == 0)
        builder.AddButton(tr.Get("punish.noTemplates", adminSlot), [](int) {}, false);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
