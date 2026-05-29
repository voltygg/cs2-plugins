#include "AdminMenu_Control.hpp"
#include "../../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../Actions/CheatCheck.hpp"
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
#include <memory>

using CS2Kit::Core::Kit;

namespace AdminSystem::Admin::Menu
{

using AdminSystem::Admin::Effects::EffectId;
using AdminSystem::Admin::Effects::EffectManager;
using CS2Kit::Menu::ChoiceOption;
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
    builder.AddButton(label, [admin, target, action](int /*slot*/) { action(admin, target); }, enabled);
}

void AddSubmenuLink(MenuBuilder& builder, const std::string& label, bool enabled, int admin, int target,
                    std::shared_ptr<::CS2Kit::Menu::Menu> (*factory)(int, int))
{
    builder.AddSubmenu(label, [admin, target, factory](int) { return factory(admin, target); }, enabled);
}

// Toggle whose ON/OFF state is read from a m_fFlags bit on the target's pawn.
void AddFlagToggle(MenuBuilder& builder, const std::string& base, bool enabled, int admin, int target, uint32_t flag,
                   void (*action)(int, int))
{
    auto& tr = Kit().Translations;
    builder.AddToggle(
        base, tr.Get("effectState.on"), tr.Get("effectState.off"),
        [target, flag](int) {
            PlayerController pc(target);
            return pc.IsValid() && (pc.GetPawnField<uint32_t>("CBaseEntity", "m_fFlags") & flag) != 0;
        },
        [admin, target, action](int) { action(admin, target); }, enabled);
}

const int HealthPresets[] = {1, 50, 100, 200, 500, 999};
const int ArmorPresets[] = {0, 50, 100, 200, 500, 999};

template <typename PresetArray>
void AddPresetChoice(MenuBuilder& builder, const std::string& title, const std::string& unit, bool enabled, int admin,
                     int target, void (*action)(int, int, int), const PresetArray& presets)
{
    using Choice = ChoiceOption<int>::Choice;
    std::vector<Choice> choices;
    choices.reserve(std::size(presets));
    for (int v : presets)
        choices.push_back({std::format("{} {}", v, unit), v});

    auto idx = std::make_shared<int>(0);
    builder.AddChoice<int>(
        title, std::move(choices), [idx](int) { return *idx; }, [idx](int, int newIdx) { *idx = newIdx; },
        [admin, target, action](int slot, const int& value) {
            action(admin, target, value);
            Kit().Menus.CloseAllMenus(slot);
        },
        enabled);
}

}  // namespace

std::shared_ptr<::CS2Kit::Menu::Menu> BuildControlMenu(int adminSlot)
{
    auto& tr = Kit().Translations;
    auto& adminMgr = Sys().Admins;
    auto& plrMgr = Kit().Players;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    bool hasB = adminMgr.HasPermission(adminSid, 'b');

    MenuBuilder builder(tr.Get("category.control"));

    // Self-only Hide toggle sits at the top of the Control list before player picks.
    builder.AddToggle(
        tr.Get("action.hide"), tr.Get("effectState.on"), tr.Get("effectState.off"),
        [adminSlot](int) { return Sys().Effects.IsActive(adminSlot, EffectId::Hide); },
        [adminSlot](int) { Effects::ToggleHide(adminSlot); }, hasB);

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
    bool canTarget = adminMgr.CanTarget(adminSid, targetSid);
    bool hasS = canTarget && adminMgr.HasPermission(adminSid, 's');
    bool hasH = canTarget && adminMgr.HasPermission(adminSid, 'h');
    bool hasK = canTarget && adminMgr.HasPermission(adminSid, 'k');

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.control"), target->GetName()));

    AddSimple(builder, tr.Get("action.slay"), hasS, adminSlot, targetSlot, &Actions::DoSlay);
    AddSimple(builder, tr.Get("action.bring"), hasS, adminSlot, targetSlot, &Actions::DoBring);
    AddSimple(builder, tr.Get("action.goto"), hasS, adminSlot, targetSlot, &Actions::DoGoto);
    AddSimple(builder, tr.Get("action.freeze"), hasS, adminSlot, targetSlot, &Actions::DoFreeze);
    AddSimple(builder, tr.Get("action.noclip"), hasS, adminSlot, targetSlot, &Actions::DoNoclip);

    // HP/Armor are inline Choice rows: A/D cycles preset values, E applies and closes.
    // The full preset submenus are still reachable via PresetSubmenu helpers when needed.
    AddPresetChoice(builder, tr.Get("action.health"), "HP", hasH, adminSlot, targetSlot, &Actions::DoSetHealth,
                    HealthPresets);
    AddPresetChoice(builder, tr.Get("action.armor"), "AP", hasH, adminSlot, targetSlot, &Actions::DoSetArmor,
                    ArmorPresets);

    AddFlagToggle(builder, tr.Get("action.godmode"), hasH, adminSlot, targetSlot, FL_GODMODE,
                  &Actions::DoToggleGodmode);

    AddSimple(builder, tr.Get("action.bury"), hasS, adminSlot, targetSlot, &Actions::DoBury);
    AddSimple(builder, tr.Get("action.unbury"), hasS, adminSlot, targetSlot, &Actions::DoUnbury);
    AddSubmenuLink(builder, tr.Get("action.changeTeam"), hasS, adminSlot, targetSlot, &BuildTeamPickerMenu);

    AddSimple(builder, tr.Get("action.callCheck"), hasK, adminSlot, targetSlot, &Actions::DoCallCheck);
    AddSimple(builder, tr.Get("action.cancelCheck"), hasK, adminSlot, targetSlot,
              [](int a, int t) { Actions::DoCancelCheck(a, t); });

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
