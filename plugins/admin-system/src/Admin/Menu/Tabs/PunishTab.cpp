#include "Admin/Menu/Tabs/PunishTab.hpp"

#include "Admin/AdminManager.hpp"
#include "Admin/Menu/Flows/LiftFlow.hpp"
#include "Admin/Menu/MenuAccess.hpp"
#include "Admin/Menu/Pickers/PlayerPicker.hpp"
#include "Admin/Menu/Flows/PunishFlow.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <string>
#include <utility>

using AdminSystem::Punishments::PunishType;
using AdminSystem::Punishments::PunishTypeInfo;
using AdminSystem::Punishments::PunishTypes;

namespace AdminSystem::Admin::Menu
{

using VoltMod::ButtonRow;
using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;

std::shared_ptr<VoltMod::Menu> BuildPunishTab(const MenuContext& ctx)
{
    App& app = ctx.Plugin;
    MenuBuilder builder(ctx.Translate("category.punish"));

    builder.Add(SubmenuRow{.Label = ctx.Translate("action.unban"),
                           .Build = [&app](int slot) { return BuildUnbanMenu(app, slot); },
                           .Enabled = Allows(app, Permission::Unban)});

    builder.Add(SubmenuRow{.Label = ctx.Translate("action.unmute"),
                           .Build = [&app](int slot) { return BuildUnmuteMenu(app, slot); },
                           .Enabled = Allows(app, Permission::Mute)});

    AppendPlayerRows(app, ctx.Admin.Slot, builder,
                     {.Open = [ctx](VoltMod::PlayerRef target) { return BuildPunishCard(ctx, target); }});

    return builder.Build();
}

std::shared_ptr<VoltMod::Menu> BuildPunishCard(const MenuContext& ctx, VoltMod::PlayerRef targetRef)
{
    App& app = ctx.Plugin;
    const int adminSlot = ctx.Admin.Slot;

    auto* target = ctx.Player(targetRef);
    if (!target)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", ctx.Translate("category.punish"), target->Name()));

    if (AnyTemplateUsable(app, adminSlot, targetRef))
    {
        builder.Submenu(ctx.Translate("punish.quickPunish"),
                        [&app, targetRef](int slot) { return BuildQuickPunishMenu(app, slot, targetRef); });
    }

    for (const PunishTypeInfo& info : PunishTypes)
    {
        const PunishType type = info.Type;
        builder.Add(ButtonRow{
            .Label = ctx.Translate(ActionTranslationKey(type)),
            .Activate = [&app, pending = PendingPunishment{.Type = type, .Target = targetRef}](
                            int slot) { StartPunishFlow(app, slot, pending); },
            .Enabled = [&app, targetRef, type](int slot) { return CanStillPunish(app, slot, targetRef, type); }});
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
