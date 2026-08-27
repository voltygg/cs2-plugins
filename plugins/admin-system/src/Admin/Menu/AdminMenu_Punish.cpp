#include "AdminMenu_Punish.hpp"

#include "../../Core/App.hpp"
#include "../AdminManager.hpp"
#include "AdminMenu_Unban.hpp"
#include "AdminMenu_Unmute.hpp"
#include "PunishFlow.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuManager.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <string>
#include <utility>

using AdminSystem::Punishments::PermissionFor;
using AdminSystem::Punishments::PunishType;

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;

std::shared_ptr<VoltMod::MenuView> BuildPunishMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;

    auto* admin = app.Runtime.Players.Get(adminSlot);
    if (!admin)
        return nullptr;

    MenuBuilder builder(tr.Get("category.punish", adminSlot));

    builder.Submenu(
        tr.Get("action.unban", adminSlot), [&app](int slot) { return BuildUnbanMenu(app, slot); },
        app.Access.HasPermission(admin->SteamId(), Permission::Unban));

    builder.Submenu(
        tr.Get("action.unmute", adminSlot), [&app](int slot) { return BuildUnmuteMenu(app, slot); },
        app.Access.HasPermission(admin->SteamId(), Permission::Mute));

    VoltMod::AppendPlayerRows(
        builder, app.Runtime.Players, adminSlot,
        [&app](int admin, int target) {
            auto actions = BuildPunishActionsMenu(app, admin, target);
            if (actions)
                app.Runtime.Menus.OpenMenu(admin, actions);
        },
        tr.Get("common.noPlayers", adminSlot));

    return builder.Build();
}

std::shared_ptr<VoltMod::MenuView> BuildPunishActionsMenu(AdminSystem::App& app, int adminSlot, int targetSlot)
{
    auto& tr = app.Runtime.Translations;
    auto& access = app.Access;
    auto& plrMgr = app.Runtime.Players;

    auto* target = plrMgr.Get(targetSlot);
    if (!target)
        return nullptr;

    auto* admin = plrMgr.Get(adminSlot);
    if (!admin)
        return nullptr;

    int64_t adminSid = admin->SteamId();
    int64_t targetSid = target->SteamId();

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.punish", adminSlot), target->Name()));

    if (AnyTemplateUsable(app, adminSlot, target->Ref()))
    {
        builder.Submenu(tr.Get("punish.quickPunish", adminSlot),
                        [&app, targetRef = target->Ref()](int slot) {
                            return BuildQuickPunishMenu(app, slot, targetRef);
                        });
    }

    for (PunishType type :
         {PunishType::Kick, PunishType::Ban, PunishType::VoiceMute, PunishType::TextMute, PunishType::Warn})
    {
        PendingPunishment pending{.Type = type, .Target = target->Ref()};
        builder.Button(
            tr.Get(ActionTranslationKey(type), adminSlot),
            [&app, pending = std::move(pending)](int slot) { StartPunishFlow(app, slot, pending); },
            access.CanActOn(adminSid, targetSid, PermissionFor(type)));
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
