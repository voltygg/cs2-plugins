#include "Admin/Menu/Pickers/TeamPicker.hpp"

#include "Admin/Actions/ActionContext.hpp"
#include "Admin/Actions/Descriptors.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <string>
#include <VoltMod/Entities/PawnOps.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Admin::Menu
{

using VoltMod::MenuBuilder;

std::shared_ptr<VoltMod::Menu> BuildTeamPicker(const MenuContext& ctx, VoltMod::PlayerRef target)
{
    App& app = ctx.Plugin;
    MenuBuilder builder(ctx.Translate("action.changeTeam"));

    auto addTeam = [&](const std::string& label, int team) {
        builder.Button(label, [&app, admin = ctx.Admin, target, team](int slot) {
            app.Actions.Run(admin, target, team, Actions::ChangeTeam);
            app.Runtime.Menus.CloseAll(slot);
        });
    };

    addTeam(ctx.Translate("team.ct"), VoltMod::TeamCT);
    addTeam(ctx.Translate("team.t"), VoltMod::TeamT);
    addTeam(ctx.Translate("team.spec"), VoltMod::TeamSpectator);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu
