#include "App.hpp"

#include <VoltMod/Api.hpp>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;

namespace MainMenu
{

void RegisterCommands(App& app)
{
    app.Runtime.Commands.Add("menu").Alias("m").Describe("Open the main menu").Run([&app](Caller c) -> Result<Reply> {
        // Players reach it mid-round, where being held still is worse than stray movement.
        app.Runtime.Menus.OpenSession(c.Slot, app.Hub.Build(c.Slot), {.FreezeMovement = false});
        return Reply::Silent();  // the menu is the feedback
    });
}

}  // namespace MainMenu
