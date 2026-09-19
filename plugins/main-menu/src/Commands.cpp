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
        app.Hub.Open(c.Slot);
        return Reply::Silent();  // the menu is the feedback
    });
}

}  // namespace MainMenu
