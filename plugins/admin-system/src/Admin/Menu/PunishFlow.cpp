#include "PunishFlow.hpp"

#include "../../Config/ConfigManager.hpp"
#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Punishments/IssuePunishment.hpp"
#include "../AdminManager.hpp"
#include "Labels.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Strings.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/Flow.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using AdminSystem::Punishments::IssuePunishment;
using AdminSystem::Punishments::IsTimed;
using AdminSystem::Punishments::PermissionFor;
using AdminSystem::Punishments::PunishType;

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;
using VoltMod::Strings;
using PunishFlowT = VoltMod::Flow<PendingPunishment>;

/** @p target is the reference the flow stored, so a target who left is refused rather than
 *  retargeted; the admin slot is the presser's, live at the moment this runs. */
bool CanStillPunish(App& app, int adminSlot, VoltMod::PlayerRef target, PunishType type)
{
    auto& players = app.Runtime.Players;
    return app.Runtime.Policy.Authorize(players.RefFor(adminSlot), target, Flag(PermissionFor(type))).has_value();
}

/** Flow validation: the target may have left (or the slot rehosts another player) and the
 *  admin's flags/immunity may have changed (e.g. !admin_reload) while the menu was open. */
static std::optional<std::string> ValidatePending(App& app, int slot, const PendingPunishment& pending)
{
    if (!app.Runtime.Players.Get(pending.Target))
        return "punish.targetLost";
    if (!CanStillPunish(app, slot, pending.Target, pending.Type))
        return "punish.notAllowed";
    return std::nullopt;
}

static void Issue(App& app, int adminSlot, PendingPunishment& pending)
{
    auto& tr = app.Runtime.Translations;
    auto* admin = app.Runtime.Players.Get(adminSlot);
    auto* target = app.Runtime.Players.Get(pending.Target);
    if (!admin || !target)
        return;

    if (!IssuePunishment(app, *admin, *target, pending.Type, pending.Reason, pending.DurationSec))
    {
        app.Chat.Reply(adminSlot, tr.Get("punish.failed", adminSlot,
                                         {{"action", tr.Get(ActionTranslationKey(pending.Type), adminSlot)}}));
    }
    else if (!app.Settings.GetChat().broadcastPunishments)
    {
        // With broadcasts on, the admin already sees the server-wide line; avoid double messaging.
        app.Chat.Reply(adminSlot, tr.Get("punish.issued", adminSlot,
                                         {{"action", tr.Get(ActionTranslationKey(pending.Type), adminSlot)},
                                          {"name", target->Name()}}));
    }
}

/** The validated confirm -> issue tail every punish path shares, for the one admin @p adminSlot. */
static PunishFlowT::Ptr MakeBaseFlow(App& app, int adminSlot, PendingPunishment pending)
{
    auto& tr = app.Runtime.Translations;
    auto type = pending.Type;
    return PunishFlowT::Create(app.Runtime.Menus, adminSlot, std::move(pending))
        ->Validate([&app, adminSlot](const PendingPunishment& p) { return ValidatePending(app, adminSlot, p); })
        ->Confirm({.Title = ConfirmTitle(tr, ActionTranslationKey(type), adminSlot),
                   .Summary =
                       [&app, adminSlot](const PendingPunishment& p) {
                           auto& translations = app.Runtime.Translations;
                           auto* target = app.Runtime.Players.Get(p.Target);
                           std::vector<std::pair<std::string, std::string>> rows;
                           rows.emplace_back(translations.Get("punish.target", adminSlot),
                                             target ? target->Name() : std::string());
                           if (IsTimed(p.Type))
                               rows.emplace_back(translations.Get("punish.duration", adminSlot),
                                                 DurationLabel(translations, p.DurationSec, adminSlot));
                           rows.emplace_back(translations.Get("punish.reason", adminSlot),
                                             Strings::TruncateUtf8(p.Reason, 40));
                           return rows;
                       },
                   .ConfirmLabel = ConfirmLabel(tr, adminSlot),
                   .CancelLabel = CancelLabel(tr, adminSlot)})
        ->Finish([&app, adminSlot](PendingPunishment& p) { Issue(app, adminSlot, p); });
}

void StartPunishFlow(AdminSystem::App& app, int adminSlot, PendingPunishment pending)
{
    auto& tr = app.Runtime.Translations;
    auto type = pending.Type;
    // The flow runs for one admin, so every step string resolves in their language here.
    auto stepTitle = [&tr, type, adminSlot](std::string_view suffixKey) {
        return std::format("{}: {}", tr.Get(ActionTranslationKey(type), adminSlot), tr.Get(suffixKey, adminSlot));
    };

    std::vector<std::pair<std::string, int>> durations;
    for (int seconds : app.Settings.GetMenuDurations())
        durations.emplace_back(DurationLabel(tr, seconds, adminSlot), seconds);

    std::vector<std::pair<std::string, std::string>> reasons;
    for (const auto& reason : app.Settings.GetPunishments().reasonPresets)
        reasons.emplace_back(reason, reason);

    MakeBaseFlow(app, adminSlot, std::move(pending))
        ->AddDurationStep({.Title = stepTitle("panel.selectDuration"),
                           .Presets = std::move(durations),
                           .Set = [](PendingPunishment& p, int seconds) { p.DurationSec = seconds; },
                           .CustomLabel = tr.Get("duration.custom", adminSlot),
                           .CustomPrompt = tr.Get("duration.customPrompt", adminSlot),
                           .Applies = [](const PendingPunishment& p) { return IsTimed(p.Type); }})
        ->AddOptionsStep(
            {.Title = stepTitle("punish.selectReason"),
             .Options = std::move(reasons),
             .Set = [](PendingPunishment& p, const std::string& label, const std::string&) { p.Reason = label; },
             .CustomLabel = tr.Get("punish.customReason", adminSlot),
             .CustomPrompt = tr.Get("punish.customReasonPrompt", adminSlot)})
        ->Start();
}

bool AnyTemplateUsable(AdminSystem::App& app, int adminSlot, VoltMod::PlayerRef target)
{
    for (const auto& tmpl : app.Settings.GetPunishmentTemplates())
    {
        if (CanStillPunish(app, adminSlot, target, tmpl.Type))
            return true;
    }
    return false;
}

std::shared_ptr<VoltMod::Menu> BuildQuickPunishMenu(AdminSystem::App& app, int adminSlot, VoltMod::PlayerRef target)
{
    auto& tr = app.Runtime.Translations;
    auto* targetPlayer = app.Runtime.Players.Get(target);
    if (!targetPlayer)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", tr.Get("punish.quickPunish", adminSlot), targetPlayer->Name()));

    int rows = 0;
    for (const auto& tmpl : app.Settings.GetPunishmentTemplates())
    {
        if (!CanStillPunish(app, adminSlot, target, tmpl.Type))
            continue;

        PendingPunishment pending{
            .Type = tmpl.Type,
            .Target = target,
            .DurationSec = tmpl.DurationSec,
            .Reason = tmpl.Reason,
        };
        // Duration and reason are preset by the template, so the flow jumps straight to confirm.
        builder.Button(std::format("{} - {}", tmpl.Name, DurationLabel(tr, tmpl.DurationSec, adminSlot)),
                       [&app, pending](int slot) { MakeBaseFlow(app, slot, pending)->Start(); });
        ++rows;
    }

    // Permissions can change between the actions menu and here; never show a dead-end empty page.
    if (rows == 0)
        builder.Text(tr.Get("punish.noTemplates", adminSlot));

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
