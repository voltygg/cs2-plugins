#include "Admin/Menu/PresetSubmenu.hpp"

#include "Admin/Actions/ActionContext.hpp"
#include "Admin/Actions/Descriptors.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Entities/PawnOps.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;

std::shared_ptr<VoltMod::Menu> BuildTeamPickerMenu(AdminSystem::App& app, VoltMod::PlayerRef admin,
                                                   VoltMod::PlayerRef target)
{
    auto& translations = app.Runtime.Translations;
    MenuBuilder builder(translations.Get("action.changeTeam", admin.Slot));

    auto addTeam = [&](const std::string& label, int team) {
        builder.Button(label, [&app, admin, target, team](int slot) {
            app.Actions.Run(admin, target, team, Actions::ChangeTeam);
            app.Runtime.Menus.CloseAll(slot);
        });
    };

    addTeam(translations.Get("team.ct", admin.Slot), VoltMod::TeamCT);
    addTeam(translations.Get("team.t", admin.Slot), VoltMod::TeamT);
    addTeam(translations.Get("team.spec", admin.Slot), VoltMod::TeamSpectator);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
