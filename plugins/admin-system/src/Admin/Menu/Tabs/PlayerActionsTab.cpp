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
static VoltMod::MenuItem CheatCheckRow(const MenuContext& ctx, VoltMod::ActionRows& rows, const RowSpec& spec,
                                       VoltMod::PlayerRef target)
{
    App& app = ctx.Plugin;
    return ToggleRow{.Label = rows.Translate(spec.LabelKey),
                     .Get = [&app, slot = target.Slot](int) { return app.CheatCheck.IsActive(slot); },
                     .Flip =
                         [&app, admin = ctx.Admin, target](int) {
                             if (app.CheatCheck.IsActive(target.Slot))
                                 Actions::CancelCheck(app, admin, target);
                             else
                                 Actions::CallCheck(app, admin, target);
                         },
                     .Enabled = rows.Allows(spec.Permission)}
        .ToItem();
}

static VoltMod::MenuItem MakeRow(const MenuContext& ctx, VoltMod::ActionRows& rows, const RowSpec& spec,
                                 VoltMod::PlayerRef target)
{
    auto action = [&](const VoltMod::Action& a) { return ctx.WhileAlive(target, a, rows.Action(spec.LabelKey, a)); };
    auto toggle = [&](auto pred, const VoltMod::Action& a) {
        return ctx.WhileAlive(target, a, rows.StateToggle(spec.LabelKey, pred, a));
    };
    auto presets = [&](const VoltMod::ActionRows::PresetSpec& preset) {
        return ctx.WhileAlive(target, preset.Action, rows.Presets(preset));
    };
    auto submenu = [&](auto build) {
        return SubmenuRow{.Label = rows.Translate(spec.LabelKey),
                          .Build = [ctx, target, build](int) { return build(ctx, target); },
                          .Enabled = rows.Allows(spec.Permission)}
            .ToItem();
    };

    switch (spec.Id)
    {
    case RowId::CheatCheck:
        return CheatCheckRow(ctx, rows, spec, target);
    case RowId::Kill:
        return action(Actions::Kill);
    case RowId::Bring:
        return action(Actions::Bring);
    case RowId::Goto:
        return action(Actions::Goto);
    case RowId::Swap:
        return submenu(BuildSwapPartnerPicker);
    case RowId::Freeze:
        return toggle(VoltMod::InMoveType(VoltMod::MoveType::None), Actions::Freeze);
    case RowId::Noclip:
        return toggle(VoltMod::InMoveType(VoltMod::MoveType::NoClip), Actions::Noclip);
    case RowId::Bury:
        return action(Actions::Bury);
    case RowId::Unbury:
        return action(Actions::Unbury);
    case RowId::ChangeTeam:
        return submenu(BuildTeamPicker);
    case RowId::Speed:
        return presets({.LabelKey = spec.LabelKey,
                        .Unit = "%",
                        .Presets = SpeedPresets,
                        .Action = Actions::SetSpeed,
                        .Index = SpeedDefault});
    case RowId::Slap:
        return action(ctx.Plugin.ActionDescriptors.Slap);
    case RowId::Health:
        return presets(
            {.LabelKey = spec.LabelKey, .Unit = "HP", .Presets = HealthPresets, .Action = Actions::SetHealth});
    case RowId::Armor:
        return presets({.LabelKey = spec.LabelKey, .Unit = "AP", .Presets = ArmorPresets, .Action = Actions::SetArmor});
    case RowId::Godmode:
        return toggle(VoltMod::HasPawnFlag(VoltMod::FL_GODMODE), Actions::Godmode);
    case RowId::Weapons:
        return submenu(BuildWeaponPicker);
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
    const auto title = ctx.CardTitle("category.playerActions", target);
    if (!title)
        return nullptr;

    MenuBuilder builder(*title);
    auto rows = ctx.Rows(target);

    AppendCatalogRows(ctx, builder, PlayerActionRows,
                      [&](const RowSpec& spec) { return MakeRow(ctx, rows, spec, target); });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
