#include "AdminMenu_Punish.hpp"

#include "../../Core/App.hpp"
#include "../AdminManager.hpp"
#include "AdminMenu_Unban.hpp"
#include "AdminMenu_Unmute.hpp"
#include "PunishFlow.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Translations.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Menu/MenuPresets.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Runtime.hpp>
#include <format>
#include <string>
#include <utility>

namespace AdminSystem::Admin::Menu
{

using namespace AdminSystem::Punishments;
using CS2Kit::Menu::MenuBuilder;

std::shared_ptr<CS2Kit::MenuView> BuildPunishMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;

    auto* admin = app.Runtime.Players.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    MenuBuilder builder(tr.Get("category.punish", adminSlot));

    builder.AddSubmenu(
        tr.Get("action.unban", adminSlot), [&app](int slot) { return BuildUnbanMenu(app, slot); },
        app.Access.HasPermission(admin->GetSteamID(), Permission::Unban));

    builder.AddSubmenu(
        tr.Get("action.unmute", adminSlot), [&app](int slot) { return BuildUnmuteMenu(app, slot); },
        app.Access.HasPermission(admin->GetSteamID(), Permission::Mute));

    CS2Kit::Menu::AppendPlayerRows(
        builder, adminSlot,
        [&app](int admin, int target) {
            auto actions = BuildPunishActionsMenu(app, admin, target);
            if (actions)
                app.Runtime.Menus.OpenMenu(admin, actions);
        },
        tr.Get("common.noPlayers", adminSlot));

    return builder.Build();
}

std::shared_ptr<CS2Kit::MenuView> BuildPunishActionsMenu(AdminSystem::App& app, int adminSlot, int targetSlot)
{
    auto& tr = app.Runtime.Translations;
    auto& access = app.Access;
    auto& plrMgr = app.Runtime.Players;

    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!target)
        return nullptr;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    int64_t targetSid = target->GetSteamID();

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.punish", adminSlot), target->GetName()));

    if (AnyTemplateUsable(app, adminSlot, targetSlot))
    {
        builder.AddSubmenu(tr.Get("punish.quickPunish", adminSlot),
                           [&app, targetSlot](int slot) { return BuildQuickPunishMenu(app, slot, targetSlot); });
    }

    for (PunishType type :
         {PunishType::Kick, PunishType::Ban, PunishType::VoiceMute, PunishType::TextMute, PunishType::Warn})
    {
        PendingPunishment pending{
            .Type = type,
            .TargetSlot = targetSlot,
            .TargetSteamId = targetSid,
            .TargetName = target->GetName(),
        };
        builder.AddButton(
            tr.Get(ActionTranslationKey(type), adminSlot),
            [&app, pending = std::move(pending)](int slot) { StartPunishFlow(app, slot, pending); },
            access.CanActOn(adminSid, targetSid, PermissionFor(type)));
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
