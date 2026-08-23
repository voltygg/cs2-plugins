#include "PresetSubmenu.hpp"

#include "../../Core/App.hpp"
#include "../Actions/ActionContext.hpp"
#include "../Actions/Descriptors.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Translations.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Runtime.hpp>
#include <CS2Kit/Sdk/PawnOps.hpp>

namespace AdminSystem::Admin::Menu
{

using CS2Kit::Core::Translations;
using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Menu::MenuManager;

std::shared_ptr<CS2Kit::MenuView> BuildTeamPickerMenu(AdminSystem::App& app, int adminSlot, int targetSlot)
{
    auto& tr = app.Runtime.Translations;
    MenuBuilder builder(tr.Get("action.changeTeam", adminSlot));

    auto addTeam = [&](const std::string& label, int team) {
        builder.AddButton(label, [&app, adminSlot, targetSlot, team](int slot) {
            Actions::Run(adminSlot, targetSlot, team, Actions::ChangeTeam);
            app.Runtime.Menus.CloseAllMenus(slot);
        });
    };

    addTeam(tr.Get("team.ct", adminSlot), CS2Kit::Sdk::TeamCT);
    addTeam(tr.Get("team.t", adminSlot), CS2Kit::Sdk::TeamT);
    addTeam(tr.Get("team.spec", adminSlot), CS2Kit::Sdk::TeamSpectator);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
