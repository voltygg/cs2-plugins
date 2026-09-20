#include "Admin/Menu/Tabs/PlayerActionsTab.hpp"

#include "Admin/Actions/Descriptors.hpp"
#include "Admin/Menu/MenuAccess.hpp"
#include "Admin/Menu/MenuCatalog.hpp"
#include "Admin/Menu/Pickers/PlayerPicker.hpp"
#include "Admin/Menu/Pickers/SwapPartnerPicker.hpp"
#include "Admin/Menu/Pickers/TeamPicker.hpp"
#include "Admin/Menu/Pickers/WeaponPicker.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Entities/PawnPredicates.hpp>
#include <VoltMod/Menu/ActionRows.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <format>
#include <string_view>
#include <utility>

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;
using VoltMod::ToggleRow;

static constexpr int HealthPresets[] = {1, 50, 100, 200, 500, 999};
static constexpr int ArmorPresets[] = {0, 50, 100, 200, 500, 999};
static constexpr int SpeedPresets[] = {10, 25, 50, 100, 150, 200, 300};

// Speed cycles both up and down from normal, so it opens anchored on 100% (no change).
static constexpr int SpeedDefault = 3;  // index of 100 in SpeedPresets

/** Call and cancel share one switch; as two buttons, Cancel was greyed out nearly always. */
static VoltMod::MenuItem CheatCheckRow(const MenuContext& ctx, VoltMod::ActionRows& rows, VoltMod::PlayerRef target)
{
    App& app = ctx.Plugin;
    return ToggleRow{.Label = rows.Translate("action.cheatCheck"),
                     .Get = [&app, slot = target.Slot](int) { return app.CheatCheck.IsActive(slot); },
                     .Flip =
                         [&app, admin = ctx.Admin, target](int) {
                             if (app.CheatCheck.IsActive(target.Slot))
                                 Actions::CancelCheck(app, admin, target);
                             else
                                 Actions::CallCheck(app, admin, target);
                         },
                     .Enabled = rows.Allows(Permission::Control)}
        .ToItem();
}

/** The row @p id names, for the target this card belongs to. */
static VoltMod::MenuItem MakeRow(const MenuContext& ctx, VoltMod::ActionRows& rows, RowId id, VoltMod::PlayerRef target)
{
    const VoltMod::EnabledCondition control = rows.Allows(Permission::Control);

    // A RequireAlive descriptor is skipped silently on a dead target, so those rows say why.
    auto live = [&](const auto& descriptor, VoltMod::MenuItem item) {
        return descriptor.RequireAlive ? WhileAlive(ctx.Plugin, ctx.Admin.Slot, target, std::move(item))
                                       : std::move(item);
    };
    auto action = [&](std::string_view key, const VoltMod::Action& a) { return live(a, rows.Action(key, a)); };
    auto toggle = [&](std::string_view key, auto pred, const VoltMod::Action& a) {
        return live(a, rows.StateToggle(key, pred, a));
    };
    auto presets = [&](const VoltMod::ActionRows::PresetSpec& spec) { return live(spec.Action, rows.Presets(spec)); };

    switch (id)
    {
    case RowId::CheatCheck:
        return CheatCheckRow(ctx, rows, target);
    case RowId::Kill:
        return action("action.kill", Actions::Kill);
    case RowId::Bring:
        return action("action.bring", Actions::Bring);
    case RowId::Goto:
        return action("action.goto", Actions::Goto);
    case RowId::Swap:
        return SubmenuRow{.Label = rows.Translate("action.swap"),
                          .Build = [ctx, target](int) { return BuildSwapPartnerPicker(ctx, target); },
                          .Enabled = control}
            .ToItem();
    case RowId::Freeze:
        return toggle("action.freeze", VoltMod::InMoveType(VoltMod::MoveType::None), Actions::Freeze);
    case RowId::Noclip:
        return toggle("action.noclip", VoltMod::InMoveType(VoltMod::MoveType::NoClip), Actions::Noclip);
    case RowId::Bury:
        return action("action.bury", Actions::Bury);
    case RowId::Unbury:
        return action("action.unbury", Actions::Unbury);
    case RowId::ChangeTeam:
        return SubmenuRow{.Label = rows.Translate("action.changeTeam"),
                          .Build = [ctx, target](int) { return BuildTeamPicker(ctx, target); },
                          .Enabled = control}
            .ToItem();
    case RowId::Speed:
        return presets({.LabelKey = "action.speed",
                        .Unit = "%",
                        .Presets = SpeedPresets,
                        .Action = Actions::SetSpeed,
                        .Index = SpeedDefault});
    case RowId::Slap:
        return action("action.slap", ctx.Plugin.ActionDescriptors.Slap);
    case RowId::Health:
        return presets(
            {.LabelKey = "action.health", .Unit = "HP", .Presets = HealthPresets, .Action = Actions::SetHealth});
    case RowId::Armor:
        return presets(
            {.LabelKey = "action.armor", .Unit = "AP", .Presets = ArmorPresets, .Action = Actions::SetArmor});
    case RowId::Godmode:
        return toggle("action.godmode", VoltMod::HasPawnFlag(VoltMod::FL_GODMODE), Actions::Godmode);
    case RowId::Weapons:
        return SubmenuRow{.Label = rows.Translate("action.giveWeapon"),
                          .Build = [ctx, target](int) { return BuildWeaponPicker(ctx, target); },
                          .Enabled = rows.Allows(Permission::Weapon)}
            .ToItem();
    default:
        return {};
    }
}

std::shared_ptr<VoltMod::Menu> BuildPlayerActionsTab(const MenuContext& ctx)
{
    MenuBuilder builder(ctx.Translate("category.playerActions"));

    AppendTargetRows(ctx, builder, [ctx](VoltMod::PlayerRef target) { return BuildPlayerActionsCard(ctx, target); });

    return builder.Build();
}

std::shared_ptr<VoltMod::Menu> BuildPlayerActionsCard(const MenuContext& ctx, VoltMod::PlayerRef target)
{
    auto* targetPlayer = ctx.Player(target);
    if (!targetPlayer)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", ctx.Translate("category.playerActions"), targetPlayer->Name()));
    auto rows = ctx.Rows(target);

    AppendCatalogRows(ctx, builder, PlayerActionRows, [&](RowId id) { return MakeRow(ctx, rows, id, target); });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
