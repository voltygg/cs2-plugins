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

std::shared_ptr<VoltMod::MenuView> BuildTeamPickerMenu(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                                       VoltMod::PlayerRef target)
{
    auto& tr = app.Runtime.Translations;
    MenuBuilder builder(tr.Get("action.changeTeam", admin.Slot));

    auto addTeam = [&](const std::string& label, int team) {
        builder.Button(label, [&app, admin, target, team](int slot) {
            app.Actions.Run(admin, target, team, Actions::ChangeTeam);
            app.Runtime.Menus.CloseAllMenus(slot);
        });
    };

    addTeam(tr.Get("team.ct", admin.Slot), VoltMod::TeamCT);
    addTeam(tr.Get("team.t", admin.Slot), VoltMod::TeamT);
    addTeam(tr.Get("team.spec", admin.Slot), VoltMod::TeamSpectator);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
