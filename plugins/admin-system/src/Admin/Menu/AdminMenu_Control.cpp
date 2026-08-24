#include "AdminMenu_Control.hpp"

#include "../../Core/App.hpp"
#include "../Actions/Descriptors.hpp"
#include "../AdminManager.hpp"
#include "../Effects/Descriptors.hpp"
#include "PresetSubmenu.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuManager.hpp>
#include <VoltMod/Menu/MenuPresets.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <VoltMod/Sdk/Entity.hpp>
#include <format>
#include <memory>

namespace AdminSystem::Admin::Menu
{

using VoltMod::Menu::MenuBuilder;
using namespace VoltMod::Sdk;

namespace
{
constexpr int HealthPresets[] = {1, 50, 100, 200, 500, 999};
constexpr int ArmorPresets[] = {0, 50, 100, 200, 500, 999};
constexpr int SpeedPresets[] = {10, 25, 50, 100, 150, 200, 300};
constexpr int SizePresets[] = {10, 25, 50, 75, 100, 150, 200};

// Speed/Size cycle both up and down from normal, so they open anchored on 100% (no change).
constexpr int SpeedDefault = 3;  // index of 100 in SpeedPresets
constexpr int SizeDefault = 4;   // index of 100 in SizePresets
}  // namespace

std::shared_ptr<VoltMod::MenuView> BuildControlMenu(AdminSystem::App& app, int adminSlot)
{
    auto& tr = app.Runtime.Translations;
    auto& access = app.Access;

    auto* admin = app.Runtime.Players.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    bool hasB = access.HasPermission(admin->GetSteamID(), Permission::Hide);

    MenuBuilder builder(tr.Get("category.control", adminSlot));

    // Self-only Hide toggle sits at the top of the Control list before player picks.
    builder.AddToggle(
        tr.Get("action.hide", adminSlot), tr.Get("effectState.on", adminSlot), tr.Get("effectState.off", adminSlot),
        [&app, adminSlot](int) { return app.Effects.IsActive(adminSlot, Effects::Hide.Id); },
        [&app, adminSlot](int) { VoltMod::ToggleEffect(app.Effects, adminSlot, adminSlot, Effects::Hide); }, hasB);

    VoltMod::Menu::AppendPlayerRows(
        builder, adminSlot,
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

    auto* target = app.Runtime.Players.GetPlayerBySlot(targetSlot);
    if (!target || !app.Runtime.Players.GetPlayerBySlot(adminSlot))
        return nullptr;

    VoltMod::MenuContext ctx{.Admin = adminSlot, .Target = targetSlot, .Effects = &app.Effects};
    bool hasS = ctx.Allowed(Flag(Permission::Control));

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.control", adminSlot), target->GetName()));
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
        .AddStateToggleRow("action.godmode", VoltMod::HasPawnFlag(VoltMod::Sdk::FL_GODMODE), Actions::Godmode)
        .AddActionRow("action.bury", Actions::Bury)
        .AddActionRow("action.unbury", Actions::Unbury);

    builder.AddSubmenu(
        ctx.Tr("action.changeTeam"),
        [&app, adminSlot, targetSlot](int) { return BuildTeamPickerMenu(app, adminSlot, targetSlot); }, hasS);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
