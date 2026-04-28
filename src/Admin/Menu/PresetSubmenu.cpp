#include "PresetSubmenu.hpp"

#include "../Actions/Team.hpp"
#include "../Actions/Vitals.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Menu::MenuManager;
using CS2Kit::Utils::Translations;

namespace
{
const int HealthPresets[] = {1, 50, 100, 200, 500, 999};
const int ArmorPresets[] = {0, 50, 100, 200, 500, 999};
}  // namespace

std::shared_ptr<::CS2Kit::Menu::Menu> BuildHealthPresetMenu(int adminSlot, int targetSlot)
{
    auto& tr = Translations::Instance();
    MenuBuilder builder(tr.Get("actionHealth"));

    for (int hp : HealthPresets)
    {
        int admin = adminSlot;
        int target = targetSlot;
        builder.AddItem(std::format("{} HP", hp), [admin, target, hp](int slot) {
            Actions::DoSetHealth(admin, target, hp);
            MenuManager::Instance().CloseAllMenus(slot);
        });
    }
    return builder.Build();
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildArmorPresetMenu(int adminSlot, int targetSlot)
{
    auto& tr = Translations::Instance();
    MenuBuilder builder(tr.Get("actionArmor"));

    for (int armor : ArmorPresets)
    {
        int admin = adminSlot;
        int target = targetSlot;
        builder.AddItem(std::format("{} AP", armor), [admin, target, armor](int slot) {
            Actions::DoSetArmor(admin, target, armor);
            MenuManager::Instance().CloseAllMenus(slot);
        });
    }
    return builder.Build();
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildTeamPickerMenu(int adminSlot, int targetSlot)
{
    auto& tr = Translations::Instance();
    MenuBuilder builder(tr.Get("actionChangeTeam"));

    int admin = adminSlot;
    int target = targetSlot;
    builder.AddItem(tr.Get("teamCt"), [admin, target](int slot) {
        Actions::DoChangeTeam(admin, target, Actions::TeamCt);
        MenuManager::Instance().CloseAllMenus(slot);
    });
    builder.AddItem(tr.Get("teamT"), [admin, target](int slot) {
        Actions::DoChangeTeam(admin, target, Actions::TeamT);
        MenuManager::Instance().CloseAllMenus(slot);
    });
    builder.AddItem(tr.Get("teamSpec"), [admin, target](int slot) {
        Actions::DoChangeTeam(admin, target, Actions::TeamSpec);
        MenuManager::Instance().CloseAllMenus(slot);
    });
    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
