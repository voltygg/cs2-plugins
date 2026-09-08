#include "AdminMenu_Punish.hpp"

#include "../../Core/App.hpp"
#include "../AdminManager.hpp"
#include "AdminMenu_Lift.hpp"
#include "MenuAccess.hpp"
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
                           .Enabled = Allows(app, Permission::Unban)});

    builder.Add(SubmenuRow{.Label = tr.Get("action.unmute", adminSlot),
                           .Build = [&app](int slot) { return BuildUnmuteMenu(app, slot); },
                           .Enabled = Allows(app, Permission::Mute)});

    AppendPlayerRows(app, adminSlot, builder, {.Open = [&app, adminSlot](VoltMod::PlayerRef target) {
                         return BuildPunishActionsMenu(app, adminSlot, target);
                     }});

    return builder.Build();
}

std::shared_ptr<VoltMod::Menu> BuildPunishActionsMenu(AdminSystem::App& app, int adminSlot,
                                                      VoltMod::PlayerRef targetRef)
{
    auto& tr = app.Runtime.Translations;
    auto& plrMgr = app.Runtime.Players;

    auto* target = plrMgr.Get(targetRef);
    if (!target || !plrMgr.Get(adminSlot))
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.punish", adminSlot), target->Name()));

    if (AnyTemplateUsable(app, adminSlot, targetRef))
    {
        builder.Submenu(tr.Get("punish.quickPunish", adminSlot),
                        [&app, targetRef](int slot) { return BuildQuickPunishMenu(app, slot, targetRef); });
    }

    for (PunishType type :
         {PunishType::Kick, PunishType::Ban, PunishType::VoiceMute, PunishType::TextMute, PunishType::Warn})
    {
        builder.Add(ButtonRow{
            .Label = tr.Get(ActionTranslationKey(type), adminSlot),
            .Activate = [&app, pending = PendingPunishment{.Type = type, .Target = targetRef}](
                            int slot) { StartPunishFlow(app, slot, pending); },
            .Enabled = [&app, targetRef, type](int slot) { return CanStillPunish(app, slot, targetRef, type); }});
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
