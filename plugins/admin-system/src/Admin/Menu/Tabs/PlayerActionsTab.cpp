#include "Admin/Menu/Tabs/PlayerActionsTab.hpp"

#include "Admin/Actions/Descriptors.hpp"
#include "Admin/AdminManager.hpp"
#include "Admin/Effects/Descriptors.hpp"
#include "Admin/Menu/MenuAccess.hpp"
#include "Admin/Menu/Pickers/PlayerPicker.hpp"
#include "Admin/Menu/Pickers/SwapPartnerPicker.hpp"
#include "Admin/Menu/Pickers/TeamPicker.hpp"
#include "Admin/Menu/Pickers/WeaponPicker.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Entities/Entity.hpp>
#include <VoltMod/Entities/PawnPredicates.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <memory>
#include <string>

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

std::shared_ptr<VoltMod::Menu> BuildPlayerActionsTab(const MenuContext& ctx)
{
    App& app = ctx.Plugin;
    const int adminSlot = ctx.Admin.Slot;

    MenuBuilder builder(ctx.Translate("category.playerActions"));

    AppendPlayerRows(app, adminSlot, builder,
                     {.Open = [ctx](VoltMod::PlayerRef target) { return BuildPlayerActionsCard(ctx, target); }});

    return builder.Build();
}

std::shared_ptr<VoltMod::Menu> BuildPlayerActionsCard(const MenuContext& ctx, VoltMod::PlayerRef target)
{
    App& app = ctx.Plugin;
    const VoltMod::PlayerRef admin = ctx.Admin;

    auto* targetPlayer = ctx.Player(target);
    if (!targetPlayer)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", ctx.Translate("category.playerActions"), targetPlayer->Name()));
    auto rows = ctx.Rows(target);
    VoltMod::EnabledCondition control = rows.Allows(Permission::Control);

    // Cheat check first: it's the most time-critical action here. One switch rather than a pair of
    // buttons, because Cancel read as greyed-out whenever no check was running, which is nearly always.
    builder.Add(ToggleRow{.Label = rows.Translate("action.cheatCheck"),
                          .Get = [&app, slot = target.Slot](int) { return app.CheatCheck.IsActive(slot); },
                          .Flip =
                              [&app, admin, target](int) {
                                  if (app.CheatCheck.IsActive(target.Slot))
                                      Actions::CancelCheck(app, admin, target);
                                  else
                                      Actions::CallCheck(app, admin, target);
                              },
                          .Enabled = control});

    builder.Add(rows.Action("action.kill", Actions::Kill))
        .Add(rows.Action("action.bring", Actions::Bring))
        .Add(rows.Action("action.goto", Actions::Goto));

    builder.Add(SubmenuRow{.Label = rows.Translate("action.swap"),
                           .Build = [ctx, target](int) { return BuildSwapPartnerPicker(ctx, target); },
                           .Enabled = control});

    builder.Add(rows.StateToggle("action.freeze", VoltMod::InMoveType(VoltMod::MoveType::None), Actions::Freeze))
        .Add(rows.StateToggle("action.noclip", VoltMod::InMoveType(VoltMod::MoveType::NoClip), Actions::Noclip))
        .Add(rows.Action("action.bury", Actions::Bury))
        .Add(rows.Action("action.unbury", Actions::Unbury));

    builder.Add(SubmenuRow{.Label = rows.Translate("action.changeTeam"),
                           .Build = [ctx, target](int) { return BuildTeamPicker(ctx, target); },
                           .Enabled = control});

    builder.Add(rows.Presets({.LabelKey = "action.speed",
                              .Unit = "%",
                              .Presets = SpeedPresets,
                              .Action = Actions::SetSpeed,
                              .Index = SpeedDefault}))
        .Add(rows.Action("action.slap", app.ActionDescriptors.Slap))
        .Add(rows.Presets(
            {.LabelKey = "action.health", .Unit = "HP", .Presets = HealthPresets, .Action = Actions::SetHealth}))
        .Add(rows.Presets(
            {.LabelKey = "action.armor", .Unit = "AP", .Presets = ArmorPresets, .Action = Actions::SetArmor}))
        .Add(rows.StateToggle("action.godmode", VoltMod::HasPawnFlag(VoltMod::FL_GODMODE), Actions::Godmode));

    builder.Add(SubmenuRow{.Label = rows.Translate("action.giveWeapon"),
                           .Build = [ctx, target](int) { return BuildWeaponPicker(ctx, target); },
                           .Enabled = rows.Allows(Permission::Weapon)});

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
