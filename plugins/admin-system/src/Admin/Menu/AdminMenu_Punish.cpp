#include "AdminMenu_Punish.hpp"

#include "../../Core/App.hpp"
#include "../AdminManager.hpp"
#include "AdminMenu_Lift.hpp"
#include "PlayerPicker.hpp"
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

using AdminSystem::Punishments::PunishType;

namespace AdminSystem::Admin::Menu
{

using VoltMod::ButtonRow;
using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;

std::shared_ptr<VoltMod::Menu> BuildPunishMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;

    auto* admin = app.Runtime.Players.Get(adminSlot);
    if (!admin)
        return nullptr;

    MenuBuilder builder(tr.Get("category.punish", adminSlot));

    builder.Add(SubmenuRow{.Label = tr.Get("action.unban", adminSlot),
                           .Build = [&app](int slot) { return BuildUnbanMenu(app, slot); },
                           .Enabled = app.Access.HasPermission(admin->SteamId(), Permission::Unban)});

    builder.Add(SubmenuRow{.Label = tr.Get("action.unmute", adminSlot),
                           .Build = [&app](int slot) { return BuildUnmuteMenu(app, slot); },
                           .Enabled = app.Access.HasPermission(admin->SteamId(), Permission::Mute)});

    AppendPlayerRows(app, adminSlot, builder, {.Pick = [&app, adminSlot](int target) {
                         auto actions = BuildPunishActionsMenu(app, adminSlot, target);
                         if (actions)
                             app.Runtime.Menus.Open(adminSlot, actions);
                     }});

    return builder.Build();
}

std::shared_ptr<VoltMod::Menu> BuildPunishActionsMenu(AdminSystem::App& app, int adminSlot, int targetSlot)
{
    auto& tr = app.Runtime.Translations;
    auto& plrMgr = app.Runtime.Players;

    auto* target = plrMgr.Get(targetSlot);
    if (!target)
        return nullptr;

    if (!plrMgr.Get(adminSlot))
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.punish", adminSlot), target->Name()));

    if (AnyTemplateUsable(app, adminSlot, target->Ref()))
    {
        builder.Submenu(tr.Get("punish.quickPunish", adminSlot), [&app, targetRef = target->Ref()](int slot) {
            return BuildQuickPunishMenu(app, slot, targetRef);
        });
    }

    for (PunishType type :
         {PunishType::Kick, PunishType::Ban, PunishType::VoiceMute, PunishType::TextMute, PunishType::Warn})
    {
        PendingPunishment pending{.Type = type, .Target = target->Ref()};
        builder.Add(ButtonRow{
            .Label = tr.Get(ActionTranslationKey(type), adminSlot),
            .Activate = [&app, pending = std::move(pending)](int slot) { StartPunishFlow(app, slot, pending); },
            .Enabled = CanStillPunish(app, adminSlot, target->Ref(), type)});
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
