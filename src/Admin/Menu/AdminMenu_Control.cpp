#include "AdminMenu_Control.hpp"
#include "../../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../Actions/CheatCheck.hpp"
#include "../Actions/Movement.hpp"
#include "../Actions/Teleport.hpp"
#include "../Actions/Vitals.hpp"
#include "../AdminManager.hpp"
#include "../Effects/EffectId.hpp"
#include "../Effects/Hide.hpp"
#include "MenuHelpers.hpp"
#include "PresetSubmenu.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/Entity.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>
#include <memory>

using CS2Kit::Core::Kit;

namespace AdminSystem::Admin::Menu
{

using AdminSystem::Admin::Effects::EffectId;
using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Utils::Translations;
using namespace CS2Kit::Sdk;

std::shared_ptr<::CS2Kit::Menu::Menu> BuildControlMenu(int adminSlot)
{
    auto& tr = Kit().Translations;
    auto& adminMgr = Sys().Admins;
    auto& plrMgr = Kit().Players;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    bool hasB = adminMgr.HasPermission(adminSid, Permission::Hide);

    MenuBuilder builder(tr.Get("category.control"));

    // Self-only Hide toggle sits at the top of the Control list before player picks.
    builder.AddToggle(
        tr.Get("action.hide"), tr.Get("effectState.on"), tr.Get("effectState.off"),
        [adminSlot](int) { return Sys().Effects.IsActive(adminSlot, EffectId::Hide); },
        [adminSlot](int) { Effects::Run(adminSlot, adminSlot, Effects::Hide); }, hasB);

    auto players = plrMgr.GetAllPlayers();
    for (auto* p : players)
    {
        if (!p)
            continue;
        int targetSlot = p->GetSlot();
        builder.AddButton(p->GetName(), [adminSlot, targetSlot](int /*s*/) {
            auto actions = BuildControlActionsMenu(adminSlot, targetSlot);
            if (actions)
                Kit().Menus.OpenMenu(adminSlot, actions);
        });
    }
    if (players.empty())
        builder.AddButton(tr.Get("common.noPlayers"), [](int) {}, false);

    return builder.Build();
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildControlActionsMenu(int adminSlot, int targetSlot)
{
    auto& tr = Kit().Translations;
    auto& adminMgr = Sys().Admins;
    auto& plrMgr = Kit().Players;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!admin || !target)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    int64_t targetSid = target->GetSteamID();
    bool hasS = adminMgr.CanActOn(adminSid, targetSid, Permission::Control);
    bool hasH = adminMgr.CanActOn(adminSid, targetSid, Permission::Health);
    bool hasK = adminMgr.CanActOn(adminSid, targetSid, Permission::CheatCheck);

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.control"), target->GetName()));

    AddAction(builder, tr.Get("action.slay"), hasS, adminSlot, targetSlot, Actions::Slay);
    AddAction(builder, tr.Get("action.bring"), hasS, adminSlot, targetSlot, Actions::Bring);
    AddAction(builder, tr.Get("action.goto"), hasS, adminSlot, targetSlot, Actions::Goto);
    AddAction(builder, tr.Get("action.freeze"), hasS, adminSlot, targetSlot, Actions::Freeze);
    AddAction(builder, tr.Get("action.noclip"), hasS, adminSlot, targetSlot, Actions::Noclip);

    // HP/Armor are inline Choice rows: A/D cycles preset values, E applies and closes.
    AddPresetChoice(builder, tr.Get("action.health"), "HP", hasH, adminSlot, targetSlot, Actions::SetHealth,
                    HealthPresets);
    AddPresetChoice(builder, tr.Get("action.armor"), "AP", hasH, adminSlot, targetSlot, Actions::SetArmor, ArmorPresets);

    AddFlagToggle(builder, tr.Get("action.godmode"), hasH, adminSlot, targetSlot, CS2Kit::Sdk::FL_GODMODE,
                  Actions::Godmode);

    AddAction(builder, tr.Get("action.bury"), hasS, adminSlot, targetSlot, Actions::Bury);
    AddAction(builder, tr.Get("action.unbury"), hasS, adminSlot, targetSlot, Actions::Unbury);
    builder.AddSubmenu(
        tr.Get("action.changeTeam"), [adminSlot, targetSlot](int) { return BuildTeamPickerMenu(adminSlot, targetSlot); },
        hasS);

    // CheatCheck call/cancel are orchestration (no broadcast / bool result), so they stay plain functions.
    builder.AddButton(
        tr.Get("action.callCheck"), [adminSlot, targetSlot](int) { Actions::CallCheck(adminSlot, targetSlot); }, hasK);
    builder.AddButton(
        tr.Get("action.cancelCheck"), [adminSlot, targetSlot](int) { Actions::CancelCheck(adminSlot, targetSlot); },
        hasK);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
