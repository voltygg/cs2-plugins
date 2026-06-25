#include "PresetSubmenu.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../Actions/ActionContext.hpp"
#include "../Actions/Descriptors.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Menu::MenuManager;
using CS2Kit::Utils::Translations;

std::shared_ptr<::CS2Kit::Menu::Menu> BuildTeamPickerMenu(int adminSlot, int targetSlot)
{
    auto& tr = Engine().Translations;
    MenuBuilder builder(tr.Get("action.changeTeam", adminSlot));

    auto addTeam = [&](const std::string& label, int team) {
        builder.AddButton(label, [adminSlot, targetSlot, team](int slot) {
            Actions::Run(adminSlot, targetSlot, team, Actions::ChangeTeam);
            Engine().Menus.CloseAllMenus(slot);
        });
    };

    addTeam(tr.Get("team.ct", adminSlot), Actions::TeamCt);
    addTeam(tr.Get("team.t", adminSlot), Actions::TeamT);
    addTeam(tr.Get("team.spec", adminSlot), Actions::TeamSpec);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
