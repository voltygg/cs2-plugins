#include "../Admin/AdminManager.hpp"
#include "../Admin/AdminMenu.hpp"
#include "../Core/App.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Runtime.hpp>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;

void RegisterAdminMenuCommand(CS2Kit::CommandManager& commands, App& app)
{
    commands.Register({
        .Name = "admin",
        .Aliases = {"a", "menu"},
        .Description = "Open the admin menu",
        .Usage = "!admin",
        .Handler =
            [&app](CommandContext& c) {
                // Any registered admin may open the menu; each category inside is gated by its own flags.
                if (!app.Admins.IsAdmin(c.Caller->GetSteamID()))
                    return CommandResult{false, "You do not have permission to use this command."};

                int slot = c.CallerSlot();

                // Panel language is registered at connect (see AdminSystemPlugin::OnPlayerConnect).
                auto menu = AdminSystem::Admin::BuildAdminMainMenu(app, slot);
                if (!menu)
                    return CommandResult{false, "Failed to open admin menu"};

                app.Runtime.Menus.OpenMenu(slot, menu);
                return CommandResult{true, ""};  // menu UI is the feedback; no chat reply needed
            },
    });
}

}  // namespace AdminSystem::Commands
