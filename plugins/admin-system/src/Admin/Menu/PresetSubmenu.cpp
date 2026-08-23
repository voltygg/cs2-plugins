#include "PresetSubmenu.hpp"

#include "../Actions/ActionContext.hpp"
#include "../Actions/Descriptors.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/App/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Sdk/PawnOps.hpp>
#include <CS2Kit/Utils/Translations.hpp>

using CS2Kit::App::Engine;

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Menu::MenuManager;
using CS2Kit::Utils::Translations;

std::shared_ptr<CS2Kit::MenuView> BuildTeamPickerMenu(int adminSlot, int targetSlot)
{
    auto& tr = Engine().Utils.Translations;
    MenuBuilder builder(tr.Get("action.changeTeam", adminSlot));

    auto addTeam = [&](const std::string& label, int team) {
        builder.AddButton(label, [adminSlot, targetSlot, team](int slot) {
            Actions::Run(adminSlot, targetSlot, team, Actions::ChangeTeam);
            Engine().Menus.CloseAllMenus(slot);
        });
    };

    addTeam(tr.Get("team.ct", adminSlot), CS2Kit::Sdk::TeamCT);
    addTeam(tr.Get("team.t", adminSlot), CS2Kit::Sdk::TeamT);
    addTeam(tr.Get("team.spec", adminSlot), CS2Kit::Sdk::TeamSpectator);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
