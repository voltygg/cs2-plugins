#include "PresetSubmenu.hpp"

#include "../../Core/App.hpp"
#include "../Actions/ActionContext.hpp"
#include "../Actions/Descriptors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Translations.hpp>
#include <VoltMod/Entities/PawnOps.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuManager.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;
using VoltMod::MenuManager;
using VoltMod::Translations;

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

    addTeam(tr.Get("team.ct", adminSlot), VoltMod::TeamCT);
    addTeam(tr.Get("team.t", adminSlot), VoltMod::TeamT);
    addTeam(tr.Get("team.spec", adminSlot), VoltMod::TeamSpectator);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
