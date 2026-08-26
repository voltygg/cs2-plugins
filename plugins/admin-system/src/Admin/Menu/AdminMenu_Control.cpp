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
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Entities/Entity.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuManager.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <format>
#include <memory>
#include <string>

using VoltMod::MenuBuilder;
using VoltMod::MenuManager;
using VoltMod::PlayerManager;
using VoltMod::Runtime;
using VoltMod::Translations;

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;

static constexpr int HealthPresets[] = {1, 50, 100, 200, 500, 999};
static constexpr int ArmorPresets[] = {0, 50, 100, 200, 500, 999};
static constexpr int SpeedPresets[] = {10, 25, 50, 100, 150, 200, 300};
static constexpr int SizePresets[] = {10, 25, 50, 75, 100, 150, 200};

// Speed/Size cycle both up and down from normal, so they open anchored on 100% (no change).
static constexpr int SpeedDefault = 3;  // index of 100 in SpeedPresets
constexpr int SizeDefault = 4;          // index of 100 in SizePresets

/** Tell the admin why a weapon action did nothing. The menu is the only way to reach these, so
 *  a silent no-op would leave them guessing whether the click registered. */
void ReportWeaponOutcome(AdminSystem::App& app, int adminSlot, Weapons::WeaponActionResult result, const char* failKey)
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

    bool hasB = access.HasPermission(admin->SteamId(), Permission::Hide);

    MenuBuilder builder(tr.Get("category.control", adminSlot));

    // Self-only Hide toggle sits at the top of the Control list before player picks.
    builder.AddToggle(
        tr.Get("action.hide", adminSlot), tr.Get("effectState.on", adminSlot), tr.Get("effectState.off", adminSlot),
        [&app, adminSlot](int) { return app.Effects.IsActive(adminSlot, Effects::Hide.Id); },
        [&app, adminSlot](int) { app.PlayerEffects.Toggle(adminSlot, adminSlot, Effects::Hide); }, hasB);

    VoltMod::AppendPlayerRows(
        builder, app.Runtime.Players, adminSlot,
        [&app](int admin, int target) {
            auto actions = BuildControlActionsMenu(app, admin, target);
            if (actions)
                app.Runtime.Menus.OpenMenu(admin, actions);
        },
        tr.Get("common.noPlayers", adminSlot));

    return builder.Build();
}

std::shared_ptr<VoltMod::MenuView> BuildControlActionsMenu(AdminSystem::App& app, int adminSlot, int targetSlot)
{
    auto& tr = app.Runtime.Translations;

    auto* target = app.Runtime.Players.Get(targetSlot);
    if (!target || !app.Runtime.Players.Get(adminSlot))
        return nullptr;

    VoltMod::MenuContext ctx{.Rt = &app.Runtime, .Admin = adminSlot, .Target = targetSlot, .Effects = &app.Effects};
    bool hasS = ctx.Allowed(Flag(Permission::Control));

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.control", adminSlot), target->Name()));
    builder.WithContext(ctx);

    // Cheat check first: it's the most time-critical action here. Call/cancel are orchestration
    // (no broadcast / bool result), so they stay plain buttons rather than Actions descriptors.
    const bool checkActive = app.CheatCheck.IsActive(targetSlot);
    builder.AddButton(
        ctx.Tr("action.callCheck"),
        [&app, adminSlot, targetSlot](int) { Actions::CallCheck(app, adminSlot, targetSlot); }, hasS);
    builder.AddButton(
        ctx.Tr("action.cancelCheck"),
        [&app, adminSlot, targetSlot](int) { Actions::CancelCheck(app, adminSlot, targetSlot); }, hasS && checkActive);

    builder.AddActionRow("action.kill", Actions::Kill)
        .AddActionRow("action.bring", Actions::Bring)
        .AddActionRow("action.goto", Actions::Goto)
        .AddStateToggleRow("action.freeze", VoltMod::InMoveType(VoltMod::MoveType::None), Actions::Freeze)
        .AddStateToggleRow("action.noclip", VoltMod::InMoveType(VoltMod::MoveType::NoClip), Actions::Noclip)
        // HP/Armor/Speed/Size are inline Choice rows: A/D cycles preset values, E applies and closes.
        .AddPresetChoiceRow("action.health", "HP", HealthPresets, Actions::SetHealth)
        .AddPresetChoiceRow("action.armor", "AP", ArmorPresets, Actions::SetArmor)
        .AddPresetChoiceRow("action.speed", "%", SpeedPresets, Actions::SetSpeed, SpeedDefault)
        .AddPresetChoiceRow("action.size", "%", SizePresets, Actions::SetSize, SizeDefault)
        .AddStateToggleRow("action.godmode", VoltMod::HasPawnFlag(VoltMod::FL_GODMODE), Actions::Godmode)
        .AddActionRow("action.bury", Actions::Bury)
        .AddActionRow("action.unbury", Actions::Unbury);

    builder.AddSubmenu(
        ctx.Tr("action.changeTeam"),
        [&app, adminSlot, targetSlot](int) { return BuildTeamPickerMenu(app, adminSlot, targetSlot); }, hasS);

    builder.AddSubmenu(
        ctx.Tr("action.giveWeapon"),
        [&app, adminSlot, targetSlot](int) { return BuildWeaponMenu(app, adminSlot, targetSlot); },
        ctx.Allowed(Flag(Permission::Weapon)));

    return builder.Build();
}

std::shared_ptr<VoltMod::MenuView> BuildWeaponMenu(AdminSystem::App& app, int adminSlot, int targetSlot)
{
    auto& tr = app.Runtime.Translations;

    auto* target = app.Runtime.Players.Get(targetSlot);
    if (!target)
        return nullptr;

    MenuBuilder builder(std::format("{}: {}", tr.Get("action.giveWeapon", adminSlot), target->Name()));

    const auto& menu = app.Config.GetWeaponMenu();
    for (const auto& weapon : menu)
    {
        builder.AddButton(weapon.Label(), [&app, adminSlot, targetSlot, item = weapon.Item](int slot) {
            ReportWeaponOutcome(app, slot, Weapons::GiveWeapon(app, adminSlot, targetSlot, item),
                                "cmd.weaponGiveFailed");
        });
    }

    if (!menu.empty())
    {
        builder.AddButton(tr.Get("action.giveRandomWeapon", adminSlot), [&app, adminSlot, targetSlot](int slot) {
            const auto& weapons = app.Config.GetWeaponMenu();
            if (weapons.empty())
                return;
            ReportWeaponOutcome(
                app, slot,
                Weapons::GiveWeapon(app, adminSlot, targetSlot, weapons[VoltMod::RandomIndex(weapons.size())].Item),
                "cmd.weaponGiveFailed");
        });
    }
    else
    {
        // Never show a dead-end empty page.
        builder.AddButton(tr.Get("action.noWeapons", adminSlot), [](int) {}, false);
    }

    builder.AddButton(tr.Get("action.strip", adminSlot), [&app, adminSlot, targetSlot](int slot) {
        ReportWeaponOutcome(app, slot, Weapons::StripWeapons(app, adminSlot, targetSlot), "cmd.weaponStripFailed");
    });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
