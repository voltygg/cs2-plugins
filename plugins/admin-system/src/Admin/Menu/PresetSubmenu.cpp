#include "PresetSubmenu.hpp"

#include "../../Core/App.hpp"
#include "../Actions/ActionContext.hpp"
#include "../Actions/Descriptors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuManager.hpp>
#include <VoltMod/Runtime.hpp>
#include <VoltMod/Sdk/Entity/PawnOps.hpp>

namespace AdminSystem::Admin::Menu
{

using VoltMod::Core::Translations;
using VoltMod::Menu::MenuBuilder;
using VoltMod::Menu::MenuManager;

std::shared_ptr<VoltMod::MenuView> BuildTeamPickerMenu(AdminSystem::App& app, int adminSlot, int targetSlot)
{
    auto& tr = app.Runtime.Translations;
    MenuBuilder builder(tr.Get("action.changeTeam", adminSlot));

    auto addTeam = [&](const std::string& label, int team) {
        builder.AddButton(label, [&app, adminSlot, targetSlot, team](int slot) {
            app.Actions.Run(adminSlot, targetSlot, team, Actions::ChangeTeam);
            app.Runtime.Menus.CloseAllMenus(slot);
        });
    };

    addTeam(tr.Get("team.ct", adminSlot), VoltMod::Sdk::TeamCT);
    addTeam(tr.Get("team.t", adminSlot), VoltMod::Sdk::TeamT);
    addTeam(tr.Get("team.spec", adminSlot), VoltMod::Sdk::TeamSpectator);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
