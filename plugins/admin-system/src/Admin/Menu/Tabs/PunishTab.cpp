#include "Admin/Menu/Tabs/PunishTab.hpp"

#include "Admin/AdminManager.hpp"
#include "Admin/Menu/Flows/LiftFlow.hpp"
#include "Admin/Menu/MenuAccess.hpp"
#include "Admin/Menu/MenuCatalog.hpp"
#include "Admin/Menu/Pickers/PlayerPicker.hpp"
#include "Admin/Menu/Flows/PunishFlow.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <algorithm>
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

/** The punishment a catalog row issues. The two tables are checked against each other by
 *  MenuCatalogTests, so an index lookup is enough here. */
static PunishType PunishTypeFor(RowId id)
{
    const auto row = std::ranges::find(PunishCardRows, id, &RowSpec::Id);
    return PunishTypes[static_cast<std::size_t>(row - PunishCardRows.begin())].Type;
}

std::shared_ptr<VoltMod::Menu> BuildPunishTab(const MenuContext& ctx)
{
    App& app = ctx.Plugin;
    MenuBuilder builder(ctx.Translate("category.punish"));

    AddIfVisible(app, ctx.Admin.Slot, builder, PunishLiftList, [&] {
        return SubmenuRow{.Label = ctx.Translate("punish.activeList"),
                          .Build = [ctx](int) { return BuildLiftMenu(ctx); }}
            .ToItem();
    });

    AppendTargetRows(ctx, builder, [ctx](VoltMod::PlayerRef target) { return BuildPunishCard(ctx, target); });

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

    AppendCatalogRows(ctx, builder, PunishCardRows, [&](RowId id) -> VoltMod::MenuItem {
        const PunishType type = PunishTypeFor(id);
        return ButtonRow{
            .Label = ctx.Translate(ActionTranslationKey(type)),
            .Activate = [&app, pending = PendingPunishment{.Type = type, .Target = targetRef}](
                            int slot) { StartPunishFlow(app, slot, pending); },
            .Enabled = [&app, targetRef, type](int slot) { return CanStillPunish(app, slot, targetRef, type); }}
            .ToItem();
    });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
