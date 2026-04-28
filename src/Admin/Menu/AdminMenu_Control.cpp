#include "AdminMenu_Control.hpp"

#include "../Actions/Movement.hpp"
#include "../Actions/Teleport.hpp"
#include "../Actions/Vitals.hpp"
#include "../AdminManager.hpp"
#include "../Effects/EffectId.hpp"
#include "../Effects/EffectManager.hpp"
#include "../Effects/Hide.hpp"
#include "PlayerPicker.hpp"
#include "PresetSubmenu.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/Entity.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

namespace AdminSystem::Admin::Menu
{

using AdminSystem::Admin::Effects::EffectId;
using AdminSystem::Admin::Effects::EffectManager;
using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Menu::MenuManager;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Utils::Translations;
using namespace CS2Kit::Sdk;

namespace
{

void AddSimple(MenuBuilder& builder, const std::string& label, bool enabled, int admin, int target,
               void (*action)(int, int))
{
    builder.AddItem(label, [admin, target, action](int /*slot*/) { action(admin, target); }, enabled);
}

void AddSubmenu(MenuBuilder& builder, const std::string& label, bool enabled, int admin, int target,
                std::shared_ptr<::CS2Kit::Menu::Menu> (*factory)(int, int))
{
    builder.AddItem(
        label,
        [admin, target, factory](int slot) {
            auto sub = factory(admin, target);
            if (sub)
                MenuManager::Instance().OpenMenu(slot, sub);
        },
        enabled);
}

// Toggle whose ON/OFF state is read from a m_fFlags bit on the target's pawn.
// Used for entries whose state lives in the engine, not in EffectManager.
void AddFlagToggle(MenuBuilder& builder, const std::string& base, bool enabled, int admin, int target, uint32_t flag,
                   void (*action)(int, int))
{
    builder.AddDynamicItem(
        [base, target, flag]() {
            PlayerController pc(target);
            bool on = pc.IsValid() && (pc.GetPawnField<uint32_t>("CBaseEntity", "m_fFlags") & flag) != 0;
            return std::format("{}: {}", base, Translations::Instance().Get(on ? "effectStateOn" : "effectStateOff"));
        },
        [admin, target, action](int /*slot*/) { action(admin, target); }, enabled);
}

}  // namespace

std::shared_ptr<::CS2Kit::Menu::Menu> BuildControlMenu(int adminSlot)
{
    auto& tr = Translations::Instance();
    auto& adminMgr = AdminManager::Instance();
    auto& plrMgr = PlayerManager::Instance();

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    bool hasB = adminMgr.HasPermission(adminSid, 'b');

    MenuBuilder builder(tr.Get("categoryControl"));

    // Self-only Hide toggle sits at the top of the Control list before player picks.
    int slot = adminSlot;
    builder.AddDynamicItem(
        [slot]() {
            bool on = EffectManager::Instance().IsActive(slot, EffectId::Hide);
            return std::format("{}: {}", Translations::Instance().Get("actionHide"),
                               Translations::Instance().Get(on ? "effectStateOn" : "effectStateOff"));
        },
        [slot](int /*s*/) { Effects::ToggleHide(slot); }, hasB);

    auto players = plrMgr.GetAllPlayers();
    for (auto* p : players)
    {
        if (!p)
            continue;
        int targetSlot = p->GetSlot();
        int admin2 = adminSlot;
        builder.AddItem(p->GetName(), [admin2, targetSlot](int /*s*/) {
            auto actions = BuildControlActionsMenu(admin2, targetSlot);
            if (actions)
                MenuManager::Instance().OpenMenu(admin2, actions);
        });
    }
    if (players.empty())
        builder.AddItem(tr.Get("noPlayers"), [](int) {}, false);

    return builder.Build();
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildControlActionsMenu(int adminSlot, int targetSlot)
{
    auto& tr = Translations::Instance();
    auto& adminMgr = AdminManager::Instance();
    auto& plrMgr = PlayerManager::Instance();

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!admin || !target)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    int64_t targetSid = target->GetSteamID();
    bool canTarget = adminMgr.CanTarget(adminSid, targetSid);
    bool hasS = canTarget && adminMgr.HasPermission(adminSid, 's');
    bool hasH = canTarget && adminMgr.HasPermission(adminSid, 'h');

    MenuBuilder builder(std::format("{}: {}", tr.Get("categoryControl"), target->GetName()));

    AddSimple(builder, tr.Get("actionSlay"), hasS, adminSlot, targetSlot, &Actions::DoSlay);
    AddSimple(builder, tr.Get("actionBring"), hasS, adminSlot, targetSlot, &Actions::DoBring);
    AddSimple(builder, tr.Get("actionGoto"), hasS, adminSlot, targetSlot, &Actions::DoGoto);
    AddSimple(builder, tr.Get("actionFreeze"), hasS, adminSlot, targetSlot, &Actions::DoFreeze);
    AddSimple(builder, tr.Get("actionNoclip"), hasS, adminSlot, targetSlot, &Actions::DoNoclip);
    AddSubmenu(builder, tr.Get("actionHealth"), hasH, adminSlot, targetSlot, &BuildHealthPresetMenu);
    AddSubmenu(builder, tr.Get("actionArmor"), hasH, adminSlot, targetSlot, &BuildArmorPresetMenu);

    AddFlagToggle(builder, tr.Get("actionGodmode"), hasH, adminSlot, targetSlot, FL_GODMODE, &Actions::DoToggleGodmode);

    AddSimple(builder, tr.Get("actionBury"), hasS, adminSlot, targetSlot, &Actions::DoBury);
    AddSimple(builder, tr.Get("actionUnbury"), hasS, adminSlot, targetSlot, &Actions::DoUnbury);
    AddSubmenu(builder, tr.Get("actionChangeTeam"), hasS, adminSlot, targetSlot, &BuildTeamPickerMenu);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
