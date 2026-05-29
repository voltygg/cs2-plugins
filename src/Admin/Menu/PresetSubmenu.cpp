#include "PresetSubmenu.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../Actions/Team.hpp"
#include "../Actions/Vitals.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

using CS2Kit::Core::Kit;

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
    auto& tr = Kit().Translations;
    MenuBuilder builder(tr.Get("action.health"));

    for (int hp : HealthPresets)
    {
        int admin = adminSlot;
        int target = targetSlot;
        builder.AddButton(std::format("{} HP", hp), [admin, target, hp](int slot) {
            Actions::DoSetHealth(admin, target, hp);
            Kit().Menus.CloseAllMenus(slot);
        });
    }
    return builder.Build();
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildArmorPresetMenu(int adminSlot, int targetSlot)
{
    auto& tr = Kit().Translations;
    MenuBuilder builder(tr.Get("action.armor"));

    for (int armor : ArmorPresets)
    {
        int admin = adminSlot;
        int target = targetSlot;
        builder.AddButton(std::format("{} AP", armor), [admin, target, armor](int slot) {
            Actions::DoSetArmor(admin, target, armor);
            Kit().Menus.CloseAllMenus(slot);
        });
    }
    return builder.Build();
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildTeamPickerMenu(int adminSlot, int targetSlot)
{
    auto& tr = Kit().Translations;
    MenuBuilder builder(tr.Get("action.changeTeam"));

    int admin = adminSlot;
    int target = targetSlot;
    builder.AddButton(tr.Get("team.ct"), [admin, target](int slot) {
        Actions::DoChangeTeam(admin, target, Actions::TeamCt);
        Kit().Menus.CloseAllMenus(slot);
    });
    builder.AddButton(tr.Get("team.t"), [admin, target](int slot) {
        Actions::DoChangeTeam(admin, target, Actions::TeamT);
        Kit().Menus.CloseAllMenus(slot);
    });
    builder.AddButton(tr.Get("team.spec"), [admin, target](int slot) {
        Actions::DoChangeTeam(admin, target, Actions::TeamSpec);
        Kit().Menus.CloseAllMenus(slot);
    });
    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
