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
    auto& translations = app.Runtime.Translations;

    auto* admin = app.Runtime.Players.Get(adminSlot);
    if (!admin)
        return nullptr;

    MenuBuilder builder(translations.Get("category.punish", adminSlot));

    builder.Add(SubmenuRow{.Label = translations.Get("action.unban", adminSlot),
                           .Build = [&app](int slot) { return BuildUnbanMenu(app, slot); },
                           .Enabled = Allows(app, Permission::Unban)});

    builder.Add(SubmenuRow{.Label = translations.Get("action.unmute", adminSlot),
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
    auto& translations = app.Runtime.Translations;
    auto& players = app.Runtime.Players;

    auto* target = players.Get(targetRef);
    if (!target || !players.Get(adminSlot))
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", translations.Get("category.punish", adminSlot), target->Name()));

    if (AnyTemplateUsable(app, adminSlot, targetRef))
    {
        builder.Submenu(translations.Get("punish.quickPunish", adminSlot),
                        [&app, targetRef](int slot) { return BuildQuickPunishMenu(app, slot, targetRef); });
    }

    for (PunishType type :
         {PunishType::Kick, PunishType::Ban, PunishType::VoiceMute, PunishType::TextMute, PunishType::Warn})
    {
        builder.Add(ButtonRow{
            .Label = translations.Get(ActionTranslationKey(type), adminSlot),
            .Activate = [&app, pending = PendingPunishment{.Type = type, .Target = targetRef}](
                            int slot) { StartPunishFlow(app, slot, pending); },
            .Enabled = [&app, targetRef, type](int slot) { return CanStillPunish(app, slot, targetRef, type); }});
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
