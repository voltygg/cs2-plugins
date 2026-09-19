#include "Commands/Commands.hpp"
#include "Core/App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;

namespace AdminSystem::Commands
{

void RegisterAdminMenuCommand(VoltMod::CommandManager& commands, App& app)
{
    commands.Add("admin").Alias("a").Describe("Open the admin menu").Run([&app](Caller c) -> Result<Reply> {
        // Any admin may open it; each category checks its own flags.
        if (!app.AdminSection.IsVisibleTo(c.Slot))
            return c.Fail("cmd.noPermission");

        if (!app.OpenAdminMenu(c.Slot))
            return c.Fail("cmd.menuFailed");
        return Reply::Silent();  // the menu is the feedback
    });
}

}  // namespace AdminSystem::Commands
