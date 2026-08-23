#include "../Admin/AdminManager.hpp"
#include "../Admin/AdminMenu.hpp"
#include "../Core/Managers.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/App/Services.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using CS2Kit::Registry;
using CS2Kit::App::Engine;

namespace
{

const bool _registered = [] {
    Registry<CommandSpec>::Add({
        .Name = "admin",
        .Aliases = {"a", "menu"},
        .Description = "Open the admin menu",
        .Usage = "!admin",
        .Handler =
            [](CommandContext& c) {
                // Any registered admin may open the menu; each category inside is gated by its own flags.
                if (!App().Admins.IsAdmin(c.Caller->GetSteamID()))
                    return CommandResult{false, "You do not have permission to use this command."};

                int slot = c.CallerSlot();

                // Panel language is registered at connect (see AdminSystemPlugin::OnPlayerConnect).
                auto menu = AdminSystem::Admin::BuildAdminMainMenu(slot);
                if (!menu)
                    return CommandResult{false, "Failed to open admin menu"};

                Engine().Menus.OpenMenu(slot, menu);
                return CommandResult{true, ""};  // menu UI is the feedback; no chat reply needed
            },
    });

    return true;
}();

}  // namespace

}  // namespace AdminSystem::Commands
