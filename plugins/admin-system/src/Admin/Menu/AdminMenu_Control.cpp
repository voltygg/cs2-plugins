#include "AdminMenu_Control.hpp"

#include "../../Core/Managers.hpp"
#include "../Actions/Descriptors.hpp"
#include "../AdminManager.hpp"
#include "../Effects/Descriptors.hpp"
#include "MenuHelpers.hpp"
#include "PresetSubmenu.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/Entity.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>
#include <memory>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Utils::Translations;
using namespace CS2Kit::Sdk;

std::shared_ptr<::CS2Kit::Menu::Menu> BuildControlMenu(int adminSlot)
{
    auto& tr = Engine().Translations;
    auto& adminMgr = App().Admins;
    auto& plrMgr = Engine().Players;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    bool hasB = adminMgr.HasPermission(adminSid, Permission::Hide);

    MenuBuilder builder(tr.Get("category.control", adminSlot));

    // Self-only Hide toggle sits at the top of the Control list before player picks.
    builder.AddToggle(
        tr.Get("action.hide", adminSlot), tr.Get("effectState.on", adminSlot), tr.Get("effectState.off", adminSlot),
        [adminSlot](int) { return App().Effects.IsActive(adminSlot, Effects::Hide.Id); },
        [adminSlot](int) { Effects::Toggle(adminSlot, adminSlot, Effects::Hide); }, hasB);

    auto players = plrMgr.GetAllPlayers();
    for (auto* p : players)
    {
        if (!p)
            continue;
        int targetSlot = p->GetSlot();
        builder.AddButton(p->GetName(), [adminSlot, targetSlot](int /*s*/) {
            auto actions = BuildControlActionsMenu(adminSlot, targetSlot);
            if (actions)
                Engine().Menus.OpenMenu(adminSlot, actions);
        });
    }
    if (players.empty())
        builder.AddButton(tr.Get("common.noPlayers", adminSlot), [](int) {}, false);

    return builder.Build();
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildControlActionsMenu(int adminSlot, int targetSlot)
{
    auto& tr = Engine().Translations;
    auto& adminMgr = App().Admins;
    auto& plrMgr = Engine().Players;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!admin || !target)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    int64_t targetSid = target->GetSteamID();
    bool hasS = adminMgr.CanActOn(adminSid, targetSid, Permission::Control);

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.control", adminSlot), target->GetName()));

    AddAction(builder, tr.Get("action.kill", adminSlot), adminSlot, targetSlot, Actions::Kill);
    AddAction(builder, tr.Get("action.bring", adminSlot), adminSlot, targetSlot, Actions::Bring);
    AddAction(builder, tr.Get("action.goto", adminSlot), adminSlot, targetSlot, Actions::Goto);
    AddAction(builder, tr.Get("action.freeze", adminSlot), adminSlot, targetSlot, Actions::Freeze);
    AddAction(builder, tr.Get("action.noclip", adminSlot), adminSlot, targetSlot, Actions::Noclip);

    // HP/Armor are inline Choice rows: A/D cycles preset values, E applies and closes.
    AddPresetChoice(builder, tr.Get("action.health", adminSlot), "HP", adminSlot, targetSlot, Actions::SetHealth,
                    HealthPresets);
    AddPresetChoice(builder, tr.Get("action.armor", adminSlot), "AP", adminSlot, targetSlot, Actions::SetArmor,
                    ArmorPresets);

    AddFlagToggle(builder, tr.Get("action.godmode", adminSlot), adminSlot, targetSlot, CS2Kit::Sdk::FL_GODMODE,
                  Actions::Godmode);

    AddAction(builder, tr.Get("action.bury", adminSlot), adminSlot, targetSlot, Actions::Bury);
    AddAction(builder, tr.Get("action.unbury", adminSlot), adminSlot, targetSlot, Actions::Unbury);
    builder.AddSubmenu(
        tr.Get("action.changeTeam", adminSlot),
        [adminSlot, targetSlot](int) { return BuildTeamPickerMenu(adminSlot, targetSlot); }, hasS);

    // CheatCheck call/cancel are orchestration (no broadcast / bool result), so they stay plain functions.
    builder.AddButton(
        tr.Get("action.callCheck", adminSlot),
        [adminSlot, targetSlot](int) { Actions::CallCheck(adminSlot, targetSlot); }, hasS);
    builder.AddButton(
        tr.Get("action.cancelCheck", adminSlot),
        [adminSlot, targetSlot](int) { Actions::CancelCheck(adminSlot, targetSlot); }, hasS);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
