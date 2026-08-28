#include "../Admin/AdminManager.hpp"
#include "../Admin/AdminMenu.hpp"
#include "../Core/App.hpp"
#include "Commands.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Menu/Html/HtmlMenuManager.hpp>
#include <VoltMod/Runtime.hpp>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;

namespace AdminSystem::Commands
{

void RegisterAdminMenuCommand(VoltMod::CommandManager& commands, App& app)
{
    commands.Add("admin")
        .Alias("a")
        .Alias("menu")
        .Describe("Open the admin menu")
        .Run([&app](Caller c) -> Result<Reply> {
            // Any registered admin may open the menu; each category inside is
            // gated by its own flags.
            if (!app.Admins.IsAdmin(c.Player->SteamId()))
                return c.Fail("cmd.noPermission");

            // Panel language is registered at connect (see
            // AdminSystemPlugin::OnPlayerConnect).
            auto menu = AdminSystem::Admin::BuildAdminMainMenu(app, c.Slot);
            if (!menu)
                return c.Fail("cmd.menuFailed");

            app.Runtime.HtmlMenus.OpenMenu(c.Slot, menu);
            return Reply::Silent();  // the menu is the feedback
        });
}

}  // namespace AdminSystem::Commands
