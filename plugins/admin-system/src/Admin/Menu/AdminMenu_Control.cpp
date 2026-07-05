#include "AdminMenu_Control.hpp"

#include "../../Core/Managers.hpp"
#include "../Actions/Descriptors.hpp"
#include "../AdminManager.hpp"
#include "../Effects/Descriptors.hpp"
#include "PresetSubmenu.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Menu/MenuPresets.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/Entity.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>
#include <memory>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;
using namespace CS2Kit::Sdk;

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

std::shared_ptr<CS2Kit::MenuView> BuildControlMenu(int adminSlot)
{
    auto& tr = Engine().Translations;
    auto& adminMgr = App().Admins;

    auto* admin = Engine().Players.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    bool hasB = adminMgr.HasPermission(admin->GetSteamID(), Permission::Hide);

    MenuBuilder builder(tr.Get("category.control", adminSlot));

    // Self-only Hide toggle sits at the top of the Control list before player picks.
    builder.AddToggle(
        tr.Get("action.hide", adminSlot), tr.Get("effectState.on", adminSlot), tr.Get("effectState.off", adminSlot),
        [adminSlot](int) { return App().Effects.IsActive(adminSlot, Effects::Hide.Id); },
        [adminSlot](int) { CS2Kit::ToggleEffect(App().Effects, adminSlot, adminSlot, Effects::Hide); }, hasB);

    CS2Kit::Menu::AppendPlayerRows(
        builder, adminSlot,
        [](int admin, int target) {
            auto actions = BuildControlActionsMenu(admin, target);
            if (actions)
                Engine().Menus.OpenMenu(admin, actions);
        },
        tr.Get("common.noPlayers", adminSlot));

    return builder.Build();
}

std::shared_ptr<CS2Kit::MenuView> BuildControlActionsMenu(int adminSlot, int targetSlot)
{
    auto& tr = Engine().Translations;

    auto* target = Engine().Players.GetPlayerBySlot(targetSlot);
    if (!target || !Engine().Players.GetPlayerBySlot(adminSlot))
        return nullptr;

    CS2Kit::MenuContext ctx{.Admin = adminSlot, .Target = targetSlot, .Effects = &App().Effects};
    bool hasS = ctx.Allowed(Flag(Permission::Control));

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.control", adminSlot), target->GetName()));
    builder.WithContext(ctx);

    // Cheat check first: it's the most time-critical action here. Call/cancel are orchestration
    // (no broadcast / bool result), so they stay plain buttons rather than Actions descriptors.
    const bool checkActive = App().CheatCheck.IsActive(targetSlot);
    builder.AddButton(
        ctx.Tr("action.callCheck"), [adminSlot, targetSlot](int) { Actions::CallCheck(adminSlot, targetSlot); }, hasS);
    builder.AddButton(
        ctx.Tr("action.cancelCheck"), [adminSlot, targetSlot](int) { Actions::CancelCheck(adminSlot, targetSlot); },
        hasS && checkActive);

    builder.AddActionRow("action.kill", Actions::Kill)
        .AddActionRow("action.bring", Actions::Bring)
        .AddActionRow("action.goto", Actions::Goto)
        .AddStateToggleRow("action.freeze", CS2Kit::InMoveType(CS2Kit::MoveType::None), Actions::Freeze)
        .AddStateToggleRow("action.noclip", CS2Kit::InMoveType(CS2Kit::MoveType::NoClip), Actions::Noclip)
        // HP/Armor/Speed/Size are inline Choice rows: A/D cycles preset values, E applies and closes.
        .AddPresetChoiceRow("action.health", "HP", HealthPresets, Actions::SetHealth)
        .AddPresetChoiceRow("action.armor", "AP", ArmorPresets, Actions::SetArmor)
        .AddPresetChoiceRow("action.speed", "%", SpeedPresets, Actions::SetSpeed, SpeedDefault)
        .AddPresetChoiceRow("action.size", "%", SizePresets, Actions::SetSize, SizeDefault)
        .AddStateToggleRow("action.godmode", CS2Kit::HasPawnFlag(CS2Kit::Sdk::FL_GODMODE), Actions::Godmode)
        .AddActionRow("action.bury", Actions::Bury)
        .AddActionRow("action.unbury", Actions::Unbury);

    builder.AddSubmenu(
        ctx.Tr("action.changeTeam"),
        [adminSlot, targetSlot](int) { return BuildTeamPickerMenu(adminSlot, targetSlot); }, hasS);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
