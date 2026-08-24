#include "../Admin/AdminManager.hpp"
#include "../Admin/AdminMenu.hpp"
#include "../Core/App.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/MenuManager.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Commands
{

using namespace VoltMod::Commands;

void RegisterAdminMenuCommand(VoltMod::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "admin",
        .Aliases = {"a", "menu"},
        .Description = "Open the admin menu",
        .Handler =
            [&app](CommandContext& c) {
                // Any registered admin may open the menu; each category inside is gated by its own flags.
                if (!app.Admins.IsAdmin(c.Caller->GetSteamID()))
                    return c.Fail("cmd.noPermission");

                int slot = c.CallerSlot();

                // Panel language is registered at connect (see AdminSystemPlugin::OnPlayerConnect).
                auto menu = AdminSystem::Admin::BuildAdminMainMenu(app, slot);
                if (!menu)
                    return c.Fail("cmd.menuFailed");

                app.Runtime.Menus.OpenMenu(slot, menu);
                return CommandResult::Silent();  // the menu is the feedback
            },
    });
}

}  // namespace AdminSystem::Commands
