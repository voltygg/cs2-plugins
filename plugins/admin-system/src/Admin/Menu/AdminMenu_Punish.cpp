#include "AdminMenu_Punish.hpp"

#include "../../Core/Managers.hpp"
#include "../AdminManager.hpp"
#include "AdminMenu_Unban.hpp"
#include "AdminMenu_Unmute.hpp"
#include "PunishFlow.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Menu/MenuPresets.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>
#include <string>
#include <utility>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Menu
{

using namespace AdminSystem::Punishments;
using CS2Kit::Menu::MenuBuilder;

std::shared_ptr<CS2Kit::MenuView> BuildPunishMenu(int adminSlot)
{
    auto& tr = Engine().Translations;

    auto* admin = Engine().Players.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    MenuBuilder builder(tr.Get("category.punish", adminSlot));

    builder.AddSubmenu(
        tr.Get("action.unban", adminSlot), [](int slot) { return BuildUnbanMenu(slot); },
        App().Admins.HasPermission(admin->GetSteamID(), Permission::Unban));

    builder.AddSubmenu(
        tr.Get("action.unmute", adminSlot), [](int slot) { return BuildUnmuteMenu(slot); },
        App().Admins.HasPermission(admin->GetSteamID(), Permission::Mute));

    CS2Kit::Menu::AppendPlayerRows(
        builder, adminSlot,
        [](int admin, int target) {
            auto actions = BuildPunishActionsMenu(admin, target);
            if (actions)
                Engine().Menus.OpenMenu(admin, actions);
        },
        tr.Get("common.noPlayers", adminSlot));

    return builder.Build();
}

std::shared_ptr<CS2Kit::MenuView> BuildPunishActionsMenu(int adminSlot, int targetSlot)
{
    auto& tr = Engine().Translations;
    auto& adminMgr = App().Admins;
    auto& plrMgr = Engine().Players;

    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!target)
        return nullptr;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    int64_t targetSid = target->GetSteamID();

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.punish", adminSlot), target->GetName()));

    if (AnyTemplateUsable(adminSlot, targetSlot))
    {
        builder.AddSubmenu(tr.Get("punish.quickPunish", adminSlot),
                           [targetSlot](int slot) { return BuildQuickPunishMenu(slot, targetSlot); });
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
            [pending = std::move(pending)](int slot) { StartPunishFlow(slot, pending); },
            adminMgr.CanActOn(adminSid, targetSid, PermissionFor(type)));
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
