#include "AdminMenu_Effects.hpp"
#include "../../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../Actions/Launch.hpp"
#include "../Actions/Teleport.hpp"
#include "../AdminManager.hpp"
#include "../Effects/Disco.hpp"
#include "../Effects/EffectManager.hpp"
#include "../Effects/Ghost.hpp"
#include "../Effects/Smite.hpp"
#include "PlayerPicker.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Menu::MenuManager;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Utils::Translations;
using Effects::EffectId;
using Effects::EffectManager;

namespace
{

void AddEffectToggle(MenuBuilder& builder, const std::string& base, bool enabled, int admin, int target, EffectId id,
                     void (*action)(int, int))
{
    auto& tr = CS2Kit::Core::Kit().Translations;
    builder.AddToggle(
        base, tr.Get("effectState.on"), tr.Get("effectState.off"),
        [target, id](int) { return Sys().Effects.IsActive(target, id); },
        [admin, target, action](int) { action(admin, target); }, enabled);
}

void AddSimple(MenuBuilder& builder, const std::string& label, bool enabled, int admin, int target,
               void (*action)(int, int))
{
    builder.AddButton(label, [admin, target, action](int /*slot*/) { action(admin, target); }, enabled);
}

}  // namespace

std::shared_ptr<::CS2Kit::Menu::Menu> BuildEffectsMenu(int adminSlot)
{
    auto& tr = CS2Kit::Core::Kit().Translations;
    return BuildPlayerPicker(adminSlot, tr.Get("category.effects"), [](int admin, int target) {
        auto actions = BuildEffectsActionsMenu(admin, target);
        if (actions)
            CS2Kit::Core::Kit().Menus.OpenMenu(admin, actions);
    });
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildEffectsActionsMenu(int adminSlot, int targetSlot)
{
    auto& tr = CS2Kit::Core::Kit().Translations;
    auto& adminMgr = Sys().Admins;
    auto& plrMgr = CS2Kit::Core::Kit().Players;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!admin || !target)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    int64_t targetSid = target->GetSteamID();
    bool hasF = adminMgr.CanTarget(adminSid, targetSid) && adminMgr.HasPermission(adminSid, 'f');
    bool hasS = adminMgr.CanTarget(adminSid, targetSid) && adminMgr.HasPermission(adminSid, 's');

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.effects"), target->GetName()));

    AddEffectToggle(builder, tr.Get("action.ghost"), hasF, adminSlot, targetSlot, EffectId::Ghost,
                    &Effects::ToggleGhost);
    AddEffectToggle(builder, tr.Get("action.disco"), hasF, adminSlot, targetSlot, EffectId::Disco,
                    &Effects::ToggleDisco);
    builder.AddButton(tr.Get("action.blind"), [](int) {}, false);  // Awaits Fade user-message infra.
    AddSimple(builder, tr.Get("action.launch"), hasS, adminSlot, targetSlot, &Actions::DoLaunch);
    AddSimple(builder, tr.Get("action.smite"), hasF, adminSlot, targetSlot, &Effects::DoSmite);

    // Swap opens a second player picker, then runs DoSwap.
    builder.AddButton(
        tr.Get("action.swap"),
        [adminSlot, targetSlot](int slot) {
            auto picker = BuildPlayerPicker(adminSlot, CS2Kit::Core::Kit().Translations.Get("common.selectSwapTarget"),
                                            [first = targetSlot](int a, int second) {
                                                Actions::DoSwap(a, first, second);
                                                CS2Kit::Core::Kit().Menus.CloseAllMenus(a);
                                            });
            if (picker)
                CS2Kit::Core::Kit().Menus.OpenMenu(slot, picker);
        },
        hasS);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
