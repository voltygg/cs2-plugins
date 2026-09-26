#include "App.hpp"

#include <VoltMod/Api.hpp>

using VoltMod::Caller;

namespace MainMenu
{

void RegisterCommands(App& app)
{
    app.Runtime.Commands.Add("menu").Alias("m").Describe("Open the main menu").Run([&app](Caller c) {
        app.Hub.Open(c.Slot);  // the menu is the feedback
    });
}

}  // namespace MainMenu
