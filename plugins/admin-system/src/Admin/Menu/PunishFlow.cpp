#include "PunishFlow.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Config.hpp"
#include "../../Core/Managers.hpp"
#include "../../Punishments/IssuePunishment.hpp"
#include "MenuHelpers.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Menu/MenuPresets.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>
#include <string>
#include <utility>
#include <vector>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Menu
{

using namespace AdminSystem::Punishments;

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Utils::StringUtils;

namespace
{

std::shared_ptr<::CS2Kit::Menu::Menu> BuildReasonStep(int adminSlot, PendingPunishment pending);
std::shared_ptr<::CS2Kit::Menu::Menu> BuildConfirmStep(int adminSlot, PendingPunishment pending);

// Cut at a UTF-8 sequence boundary so typed Cyrillic reasons stay valid center-HTML.
std::string TruncateForDisplay(const std::string& text, std::size_t maxBytes)
{
    if (text.size() <= maxBytes)
        return text;
    std::size_t end = maxBytes;
    while (end > 0 && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80)
        --end;
    return text.substr(0, end) + "...";
}

/** Localized "{n} {unit}" (largest exactly-dividing unit), or `duration.perm` for 0. */
std::string FormatDurationLabel(int seconds, int slot)
{
    auto& tr = Engine().Translations;
    if (seconds <= 0)
        return tr.Get("duration.perm", slot);

    struct Unit
    {
        int Seconds;
        const char* Key;
    };
    static constexpr Unit Units[] = {
        {86400, "duration.unitDays"},
        {3600, "duration.unitHours"},
        {60, "duration.unitMinutes"},
    };
    for (const auto& unit : Units)
    {
        if (seconds % unit.Seconds == 0)
            return std::format("{} {}", seconds / unit.Seconds, tr.Get(unit.Key, slot));
    }
    return std::format("{} {}", seconds, tr.Get("duration.unitSeconds", slot));
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildDurationStep(int adminSlot, PendingPunishment pending)
{
    auto& tr = Engine().Translations;
    std::string action = tr.Get(ActionTranslationKey(pending.Type), adminSlot);

    const auto& durations = App().Config.GetMenuDurations();
    std::vector<std::pair<std::string, int>> presets;
    presets.reserve(durations.size());
    for (int seconds : durations)
        presets.emplace_back(FormatDurationLabel(seconds, adminSlot), seconds);

    auto onPick = [pending](int slot, int seconds) {
        auto next = pending;
        next.DurationSec = seconds;
        Engine().Menus.OpenMenu(slot, BuildReasonStep(slot, std::move(next)));
    };

    return ::CS2Kit::Menu::BuildDurationPicker(
        adminSlot, std::format("{}: {}", action, tr.Get("panel.selectDuration", adminSlot)), presets, std::move(onPick),
        tr.Get("duration.custom", adminSlot), tr.Get("duration.customPrompt", adminSlot), 32);
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildReasonStep(int adminSlot, PendingPunishment pending)
{
    auto& tr = Engine().Translations;
    std::string action = tr.Get(ActionTranslationKey(pending.Type), adminSlot);

    MenuBuilder builder(std::format("{}: {}", action, tr.Get("punish.selectReason", adminSlot)));

    for (const auto& preset : App().Config.GetPunishments().reasonPresets)
    {
        builder.AddButton(preset, [pending, preset](int slot) {
            auto next = pending;
            next.Reason = preset;
            Engine().Menus.OpenMenu(slot, BuildConfirmStep(slot, std::move(next)));
        });
    }

    builder.AddInput(
        tr.Get("punish.customReason", adminSlot), tr.Get("punish.customReasonPrompt", adminSlot),
        [](int) { return std::string(); },
        [pending](int slot, std::string_view text) {
            std::string reason = StringUtils::Trim(std::string(text));
            if (reason.empty())
                return false;  // re-prompt
            auto next = pending;
            next.Reason = std::move(reason);
            Engine().Menus.OpenMenu(slot, BuildConfirmStep(slot, std::move(next)));
            return true;
        },
        64);

    return builder.Build();
}

void ConfirmAndIssue(int adminSlot, const PendingPunishment& pending)
{
    auto& tr = Engine().Translations;
    auto& plrMgr = Engine().Players;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    if (!admin)
        return;

    // The target may have disconnected (or the slot may host a different player) since selection.
    auto* target = plrMgr.GetPlayerBySlot(pending.TargetSlot);
    if (!target || target->GetSteamID() != pending.TargetSteamId)
    {
        App().Chat.Reply(adminSlot, tr.Get("punish.targetLost", adminSlot));
        Engine().Menus.CloseAllMenus(adminSlot);
        return;
    }

    // Flags or immunity may have changed (e.g. !admin_reload) while the menu was open.
    if (!CanActOnSlot(adminSlot, pending.TargetSlot, PermissionFor(pending.Type)))
    {
        App().Chat.Reply(adminSlot, tr.Get("punish.notAllowed", adminSlot));
        Engine().Menus.CloseAllMenus(adminSlot);
        return;
    }

    if (!IssuePunishment(*admin, *target, pending.Type, pending.Reason, pending.DurationSec))
    {
        App().Chat.Reply(adminSlot, StringUtils::SubstituteTokens(
                                        tr.Get("punish.failed", adminSlot),
                                        {{"action", tr.Get(ActionTranslationKey(pending.Type), adminSlot)}}));
    }
    else if (!App().Config.GetChat().broadcastPunishments)
    {
        // With broadcasts on, the admin already sees the server-wide line; avoid double messaging.
        App().Chat.Reply(
            adminSlot, StringUtils::SubstituteTokens(tr.Get("punish.issued", adminSlot),
                                                     {{"action", tr.Get(ActionTranslationKey(pending.Type), adminSlot)},
                                                      {"name", pending.TargetName}}));
    }
    Engine().Menus.CloseAllMenus(adminSlot);
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildConfirmStep(int adminSlot, PendingPunishment pending)
{
    auto& tr = Engine().Translations;
    std::string action = tr.Get(ActionTranslationKey(pending.Type), adminSlot);

    MenuBuilder builder(std::format("{}: {}", tr.Get("punish.confirmTitle", adminSlot), action));
    builder.AddText(std::format("{}: {}", tr.Get("punish.target", adminSlot), pending.TargetName));
    if (IsTimed(pending.Type))
    {
        builder.AddText(std::format("{}: {}", tr.Get("punish.duration", adminSlot),
                                    FormatDurationLabel(pending.DurationSec, adminSlot)));
    }
    builder.AddText(std::format("{}: {}", tr.Get("punish.reason", adminSlot), TruncateForDisplay(pending.Reason, 40)));

    builder.AddButton(tr.Get("punish.confirm", adminSlot), [pending](int slot) { ConfirmAndIssue(slot, pending); });
    builder.AddButton(tr.Get("punish.cancel", adminSlot), [](int slot) { Engine().Menus.CloseAllMenus(slot); });

    return builder.Build();
}

}  // namespace

std::shared_ptr<::CS2Kit::Menu::Menu> BuildFirstStep(int adminSlot, PendingPunishment pending)
{
    return IsTimed(pending.Type) ? BuildDurationStep(adminSlot, std::move(pending))
                                 : BuildReasonStep(adminSlot, std::move(pending));
}

bool AnyTemplateUsable(int adminSlot, int targetSlot)
{
    for (const auto& tmpl : App().Config.GetPunishmentTemplates())
    {
        if (CanActOnSlot(adminSlot, targetSlot, PermissionFor(tmpl.Type)))
            return true;
    }
    return false;
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildQuickPunishMenu(int adminSlot, int targetSlot)
{
    auto& tr = Engine().Translations;
    auto* target = Engine().Players.GetPlayerBySlot(targetSlot);
    if (!target)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", tr.Get("punish.quickPunish", adminSlot), target->GetName()));

    int rows = 0;
    for (const auto& tmpl : App().Config.GetPunishmentTemplates())
    {
        if (!CanActOnSlot(adminSlot, targetSlot, PermissionFor(tmpl.Type)))
            continue;

        PendingPunishment pending{
            .Type = tmpl.Type,
            .TargetSlot = targetSlot,
            .TargetSteamId = target->GetSteamID(),
            .TargetName = target->GetName(),
            .DurationSec = tmpl.DurationSec,
            .Reason = tmpl.Reason,
        };
        builder.AddButton(std::format("{} - {}", tmpl.Name, FormatDurationLabel(tmpl.DurationSec, adminSlot)),
                          [pending](int slot) { Engine().Menus.OpenMenu(slot, BuildConfirmStep(slot, pending)); });
        ++rows;
    }

    // Permissions can change between the actions menu and here; never show a dead-end empty page.
    if (rows == 0)
        builder.AddButton(tr.Get("punish.noTemplates", adminSlot), [](int) {}, false);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
