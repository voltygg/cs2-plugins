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
#include <VoltMod/Menu/MenuManager.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <memory>
#include <string>

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;

static constexpr int HealthPresets[] = {1, 50, 100, 200, 500, 999};
static constexpr int ArmorPresets[] = {0, 50, 100, 200, 500, 999};
static constexpr int SpeedPresets[] = {10, 25, 50, 100, 150, 200, 300};
static constexpr int SizePresets[] = {10, 25, 50, 75, 100, 150, 200};

// Speed/Size cycle both up and down from normal, so they open anchored on 100% (no change).
static constexpr int SpeedDefault = 3;  // index of 100 in SpeedPresets
static constexpr int SizeDefault = 4;   // index of 100 in SizePresets

/** Tell the admin why a weapon action did nothing. The menu is the only way to reach these, so
 *  a silent no-op would leave them guessing whether the click registered. */
static void ReportWeaponOutcome(AdminSystem::App& app, int adminSlot, Weapons::WeaponActionResult result, const char* failKey)
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

std::shared_ptr<VoltMod::MenuView> BuildControlMenu(AdminSystem::App& app, int adminSlot)
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
    builder.Toggle(
        tr.Get("action.hide", adminSlot), tr.Get("effectState.on", adminSlot), tr.Get("effectState.off", adminSlot),
        [&app, adminSlot](int) { return app.Effects.IsActive(adminSlot, app.EffectDescriptors.Hide.Id); },
        [&app, adminRef](int) { app.PlayerEffects.Toggle(adminRef, adminRef, app.EffectDescriptors.Hide); }, hasB);

    VoltMod::AppendPlayerRows(
        builder, app.Runtime.Players, adminSlot,
        [&app](int viewerSlot, int targetSlot) {
            // Both slots are current at press time, so this is where they become references.
            auto& players = app.Runtime.Players;
            auto actions = BuildControlActionsMenu(app, players.RefFor(viewerSlot), players.RefFor(targetSlot));
            if (actions)
                app.Runtime.Menus.OpenMenu(viewerSlot, actions);
        },
        tr.Get("common.noPlayers", adminSlot));

    return builder.Build();
}

std::shared_ptr<VoltMod::MenuView> BuildControlActionsMenu(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                                           VoltMod::PlayerRef target)
{
    auto& tr = app.Runtime.Translations;

    auto* adminPlayer = app.Runtime.Players.Get(admin);
    auto* targetPlayer = app.Runtime.Players.Get(target);
    if (!targetPlayer || !adminPlayer)
        return nullptr;

    MenuBuilder builder(app.Runtime.Menus,
                        std::format("{}: {}", tr.Get("category.control", admin.Slot), targetPlayer->Name()));
    builder.For(admin, target, &app.Effects);
    bool hasS = builder.Allowed(Flag(Permission::Control));

    // Cheat check first: it's the most time-critical action here. Call/cancel are orchestration
    // (no broadcast / bool result), so they stay plain buttons rather than Actions descriptors.
    const bool checkActive = app.CheatCheck.IsActive(target.Slot);
    builder.Button(
        builder.Tr("action.callCheck"), [&app, admin, target](int) { Actions::CallCheck(app, admin, target); }, hasS);
    builder.Button(
        builder.Tr("action.cancelCheck"), [&app, admin, target](int) { Actions::CancelCheck(app, admin, target); },
        hasS && checkActive);

    builder.Row("action.kill", Actions::Kill)
        .Row("action.bring", Actions::Bring)
        .Row("action.goto", Actions::Goto)
        .StateToggle("action.freeze", VoltMod::InMoveType(VoltMod::MoveType::None), Actions::Freeze)
        .StateToggle("action.noclip", VoltMod::InMoveType(VoltMod::MoveType::NoClip), Actions::Noclip)
        // HP/Armor/Speed/Size are inline Choice rows: A/D cycles preset values, E applies and closes.
        .Presets("action.health", "HP", HealthPresets, Actions::SetHealth)
        .Presets("action.armor", "AP", ArmorPresets, Actions::SetArmor)
        .Presets("action.speed", "%", SpeedPresets, Actions::SetSpeed, SpeedDefault)
        .Presets("action.size", "%", SizePresets, app.ActionDescriptors.SetSize, SizeDefault)
        .StateToggle("action.godmode", VoltMod::HasPawnFlag(VoltMod::FL_GODMODE), Actions::Godmode)
        .Row("action.bury", Actions::Bury)
        .Row("action.unbury", Actions::Unbury);

    builder.Submenu(
        builder.Tr("action.changeTeam"), [&app, admin, target](int) { return BuildTeamPickerMenu(app, admin, target); },
        hasS);

    builder.Submenu(
        builder.Tr("action.giveWeapon"), [&app, admin, target](int) { return BuildWeaponMenu(app, admin, target); },
        builder.Allowed(Flag(Permission::Weapon)));

    return builder.Build();
}

std::shared_ptr<VoltMod::MenuView> BuildWeaponMenu(AdminSystem::App& app, VoltMod::PlayerRef admin,
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
        builder.Button(tr.Get("action.noWeapons", admin.Slot), [](int) {}, false);
    }

    builder.Button(tr.Get("action.strip", admin.Slot), [&app, admin, target](int slot) {
        ReportWeaponOutcome(app, slot, Weapons::StripWeapons(app, admin, target), "cmd.weaponStripFailed");
    });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
