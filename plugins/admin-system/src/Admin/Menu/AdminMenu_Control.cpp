#include "AdminMenu_Control.hpp"

#include "../../Core/App.hpp"
#include "../../Core/ChatService.hpp"
#include "../../Weapons/WeaponActions.hpp"
#include "../../Weapons/WeaponCatalog.hpp"
#include "../Actions/Descriptors.hpp"
#include "../AdminManager.hpp"
#include "../Effects/Descriptors.hpp"
#include "PresetSubmenu.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Random.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Entities/Entity.hpp>
#include <VoltMod/Entities/PawnPredicates.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuHost.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <memory>
#include <string>
#include <string_view>

namespace AdminSystem::Admin::Menu
{

using VoltMod::ButtonRow;
using VoltMod::MenuBuilder;
using VoltMod::SubmenuRow;
using VoltMod::ToggleRow;

static constexpr int HealthPresets[] = {1, 50, 100, 200, 500, 999};
static constexpr int ArmorPresets[] = {0, 50, 100, 200, 500, 999};
static constexpr int SpeedPresets[] = {10, 25, 50, 100, 150, 200, 300};
static constexpr int SizePresets[] = {10, 25, 50, 75, 100, 150, 200};

// Speed/Size cycle both up and down from normal, so they open anchored on 100% (no change).
static constexpr int SpeedDefault = 3;  // index of 100 in SpeedPresets
static constexpr int SizeDefault = 4;   // index of 100 in SizePresets

/** Tell the admin why a weapon action did nothing. The menu is the only way to reach these, so
 *  a silent no-op would leave them guessing whether the click registered. */
static void ReportWeaponOutcome(AdminSystem::App& app, int adminSlot, Weapons::WeaponActionResult result,
                                std::string_view failKey)
{
    auto& tr = app.Runtime.Translations;
    switch (result)
    {
    case Weapons::WeaponActionResult::TargetDead:
        app.Chat.Reply(adminSlot, tr.Get("cmd.weaponTargetDead", adminSlot));
        break;
    case Weapons::WeaponActionResult::EngineRefused:
        app.Chat.Reply(adminSlot, tr.Get(failKey, adminSlot));
        break;
    case Weapons::WeaponActionResult::NotAllowed:  // the dispatcher's policy already replied
    case Weapons::WeaponActionResult::Ok:
        break;
    }
}

std::shared_ptr<VoltMod::Menu> BuildControlMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    auto& access = app.Access;

    auto* admin = app.Runtime.Players.Get(adminSlot);
    if (!admin)
        return nullptr;

    const VoltMod::PlayerRef adminRef = admin->Ref();
    bool hasB = access.HasPermission(admin->SteamId(), Permission::Hide);

    MenuBuilder builder(tr.Get("category.control", adminSlot));

    // Self-only Hide toggle sits at the top of the Control list before player picks.
    builder.Add(ToggleRow{
        .Label = tr.Get("action.hide", adminSlot),
        .On = tr.Get("effectState.on", adminSlot),
        .Off = tr.Get("effectState.off", adminSlot),
        .Get = [&app, adminSlot](int) { return app.Effects.IsActive(adminSlot, app.EffectDescriptors.Hide.Id); },
        .Flip = [&app, adminRef](int) { app.PlayerEffects.Toggle(adminRef, adminRef, app.EffectDescriptors.Hide); },
        .Enabled = hasB});

    VoltMod::AppendPlayerRows(builder, app.Runtime.Players,
                              {.Pick =
                                   [&app, adminSlot](int targetSlot) {
                                       // The target slot is current at press time, so this is
                                       // where it becomes a reference.
                                       auto& players = app.Runtime.Players;
                                       auto actions = BuildControlActionsMenu(app, players.RefFor(adminSlot),
                                                                              players.RefFor(targetSlot));
                                       if (actions)
                                           app.Menus().OpenMenu(adminSlot, actions);
                                   },
                               .EmptyLabel = tr.Get("common.noPlayers", adminSlot)});

    return builder.Build();
}

std::shared_ptr<VoltMod::Menu> BuildControlActionsMenu(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                                       VoltMod::PlayerRef target)
{
    auto& tr = app.Runtime.Translations;

    auto* adminPlayer = app.Runtime.Players.Get(admin);
    auto* targetPlayer = app.Runtime.Players.Get(target);
    if (!targetPlayer || !adminPlayer)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.control", admin.Slot), targetPlayer->Name()));
    auto rows = app.MenuRows(admin, target);
    bool hasS = rows.Allowed(Flag(Permission::Control));

    // Cheat check first: it's the most time-critical action here. Call/cancel are orchestration
    // (no broadcast / bool result), so they stay plain buttons rather than Actions descriptors.
    const bool checkActive = app.CheatCheck.IsActive(target.Slot);
    builder.Add(ButtonRow{.Label = rows.Tr("action.callCheck"),
                          .Activate = [&app, admin, target](int) { Actions::CallCheck(app, admin, target); },
                          .Enabled = hasS});
    builder.Add(ButtonRow{.Label = rows.Tr("action.cancelCheck"),
                          .Activate = [&app, admin, target](int) { Actions::CancelCheck(app, admin, target); },
                          .Enabled = hasS && checkActive});

    builder.Add(rows.Action("action.kill", Actions::Kill))
        .Add(rows.Action("action.bring", Actions::Bring))
        .Add(rows.Action("action.goto", Actions::Goto))
        .Add(rows.StateToggle("action.freeze", VoltMod::InMoveType(VoltMod::MoveType::None), Actions::Freeze))
        .Add(rows.StateToggle("action.noclip", VoltMod::InMoveType(VoltMod::MoveType::NoClip), Actions::Noclip))
        // HP/Armor/Speed/Size are inline Choice rows: A/D cycles preset values and E applies,
        // leaving the menu open so a value can be adjusted and applied again.
        .Add(rows.Presets(
            {.LabelKey = "action.health", .Unit = "HP", .Presets = HealthPresets, .Action = Actions::SetHealth}))
        .Add(rows.Presets(
            {.LabelKey = "action.armor", .Unit = "AP", .Presets = ArmorPresets, .Action = Actions::SetArmor}))
        .Add(rows.Presets({.LabelKey = "action.speed",
                           .Unit = "%",
                           .Presets = SpeedPresets,
                           .Action = Actions::SetSpeed,
                           .Index = SpeedDefault}))
        .Add(rows.Presets({.LabelKey = "action.size",
                           .Unit = "%",
                           .Presets = SizePresets,
                           .Action = app.ActionDescriptors.SetSize,
                           .Index = SizeDefault}))
        .Add(rows.StateToggle("action.godmode", VoltMod::HasPawnFlag(VoltMod::FL_GODMODE), Actions::Godmode))
        .Add(rows.Action("action.bury", Actions::Bury))
        .Add(rows.Action("action.unbury", Actions::Unbury));

    builder.Add(SubmenuRow{.Label = rows.Tr("action.changeTeam"),
                           .Build = [&app, admin, target](int) { return BuildTeamPickerMenu(app, admin, target); },
                           .Enabled = hasS});

    builder.Add(SubmenuRow{.Label = rows.Tr("action.giveWeapon"),
                           .Build = [&app, admin, target](int) { return BuildWeaponMenu(app, admin, target); },
                           .Enabled = rows.Allowed(Flag(Permission::Weapon))});

    return builder.Build();
}

std::shared_ptr<VoltMod::Menu> BuildWeaponMenu(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                               VoltMod::PlayerRef target)
{
    auto& tr = app.Runtime.Translations;

    auto* targetPlayer = app.Runtime.Players.Get(target);
    if (!targetPlayer)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", tr.Get("action.giveWeapon", admin.Slot), targetPlayer->Name()));

    const auto& menu = app.Config.GetWeaponMenu();
    for (const auto& weapon : menu)
    {
        builder.Button(weapon.Label(), [&app, admin, target, item = weapon.Item](int slot) {
            ReportWeaponOutcome(app, slot, Weapons::GiveWeapon(app, admin, target, item), "cmd.weaponGiveFailed");
        });
    }

    if (!menu.empty())
    {
        builder.Button(tr.Get("action.giveRandomWeapon", admin.Slot), [&app, admin, target](int slot) {
            const auto& weapons = app.Config.GetWeaponMenu();
            if (weapons.empty())
                return;
            ReportWeaponOutcome(
                app, slot, Weapons::GiveWeapon(app, admin, target, weapons[VoltMod::RandomIndex(weapons.size())].Item),
                "cmd.weaponGiveFailed");
        });
    }
    else
    {
        // Never show a dead-end empty page.
        builder.Add(ButtonRow{.Label = tr.Get("action.noWeapons", admin.Slot), .Enabled = false});
    }

    builder.Button(tr.Get("action.strip", admin.Slot), [&app, admin, target](int slot) {
        ReportWeaponOutcome(app, slot, Weapons::StripWeapons(app, admin, target), "cmd.weaponStripFailed");
    });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
