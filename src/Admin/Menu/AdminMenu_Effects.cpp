#include "AdminMenu_Effects.hpp"

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

std::string ToggleLabel(const std::string& base, bool on)
{
    auto& tr = Translations::Instance();
    return std::format("{}: {}", base, tr.Get(on ? "effectStateOn" : "effectStateOff"));
}

void AddToggle(MenuBuilder& builder, const std::string& base, bool enabled, int admin, int target, EffectId id,
               void (*action)(int, int))
{
    builder.AddDynamicItem(
        [base, target, id]() { return ToggleLabel(base, EffectManager::Instance().IsActive(target, id)); },
        [admin, target, action](int /*slot*/) { action(admin, target); },
        enabled);
}

void AddSimple(MenuBuilder& builder, const std::string& label, bool enabled, int admin, int target,
               void (*action)(int, int))
{
    builder.AddItem(
        label,
        [admin, target, action](int /*slot*/) { action(admin, target); },
        enabled);
}

}  // namespace

std::shared_ptr<::CS2Kit::Menu::Menu> BuildEffectsMenu(int adminSlot)
{
    auto& tr = Translations::Instance();
    return BuildPlayerPicker(adminSlot, tr.Get("categoryEffects"), [](int admin, int target) {
        auto actions = BuildEffectsActionsMenu(admin, target);
        if (actions)
            MenuManager::Instance().OpenMenu(admin, actions);
    });
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildEffectsActionsMenu(int adminSlot, int targetSlot)
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
    bool hasF = adminMgr.CanTarget(adminSid, targetSid) && adminMgr.HasPermission(adminSid, 'f');
    bool hasS = adminMgr.CanTarget(adminSid, targetSid) && adminMgr.HasPermission(adminSid, 's');

    MenuBuilder builder(std::format("{}: {}", tr.Get("categoryEffects"), target->GetName()));

    AddToggle(builder, tr.Get("actionGhost"), hasF, adminSlot, targetSlot, EffectId::Ghost, &Effects::ToggleGhost);
    AddToggle(builder, tr.Get("actionDisco"), hasF, adminSlot, targetSlot, EffectId::Disco, &Effects::ToggleDisco);
    builder.AddItem(tr.Get("actionBlind"), [](int) {}, false);  // Awaits Fade user-message infra.
    AddSimple(builder, tr.Get("actionLaunch"), hasS, adminSlot, targetSlot, &Actions::DoLaunch);
    AddSimple(builder, tr.Get("actionSmite"), hasF, adminSlot, targetSlot, &Effects::DoSmite);

    // Swap opens a second player picker, then runs DoSwap.
    {
        int admin = adminSlot;
        int first = targetSlot;
        builder.AddItem(
            tr.Get("actionSwap"),
            [admin, first](int slot) {
                auto picker = BuildPlayerPicker(admin, Translations::Instance().Get("selectSwapTarget"),
                                                [first](int a, int second) {
                                                    Actions::DoSwap(a, first, second);
                                                    MenuManager::Instance().CloseAllMenus(a);
                                                });
                if (picker)
                    MenuManager::Instance().OpenMenu(slot, picker);
            },
            hasS);
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
