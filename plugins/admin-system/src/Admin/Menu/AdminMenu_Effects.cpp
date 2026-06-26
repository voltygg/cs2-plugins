#include "AdminMenu_Effects.hpp"

#include "../../Core/Managers.hpp"
#include "../Actions/Descriptors.hpp"
#include "../AdminManager.hpp"
#include "../Effects/Descriptors.hpp"
#include "MenuHelpers.hpp"
#include "PlayerPicker.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Players::PlayerManager;
using CS2Kit::Utils::Translations;

std::shared_ptr<::CS2Kit::Menu::Menu> BuildEffectsMenu(int adminSlot)
{
    auto& tr = Engine().Translations;
    return BuildPlayerPicker(adminSlot, tr.Get("category.effects", adminSlot), [](int admin, int target) {
        auto actions = BuildEffectsActionsMenu(admin, target);
        if (actions)
            Engine().Menus.OpenMenu(admin, actions);
    });
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildEffectsActionsMenu(int adminSlot, int targetSlot)
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

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.effects", adminSlot), target->GetName()));

    AddEffectToggle(builder, tr.Get("action.ghost", adminSlot), adminSlot, targetSlot, Effects::Ghost);
    AddEffectToggle(builder, tr.Get("action.disco", adminSlot), adminSlot, targetSlot, Effects::Disco);
    AddAction(builder, tr.Get("action.launch", adminSlot), adminSlot, targetSlot, Actions::Launch);
    AddAction(builder, tr.Get("action.smite", adminSlot), adminSlot, targetSlot, Actions::Smite);

    // Swap opens a second player picker, then runs the dual-target Swap.
    builder.AddButton(
        tr.Get("action.swap", adminSlot),
        [adminSlot, targetSlot](int slot) {
            auto picker = BuildPlayerPicker(adminSlot, Engine().Translations.Get("common.selectSwapTarget", adminSlot),
                                            [first = targetSlot](int a, int second) {
                                                Actions::Swap(a, first, second);
                                                Engine().Menus.CloseAllMenus(a);
                                            });
            if (picker)
                Engine().Menus.OpenMenu(slot, picker);
        },
        hasS);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
