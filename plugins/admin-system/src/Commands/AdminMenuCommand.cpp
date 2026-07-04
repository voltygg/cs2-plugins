#include "AdminMenuCommand.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Admin/AdminMenu.hpp"
#include "../Core/Managers.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>

using CS2Kit::Core::Engine;

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace CS2Kit::Menu;
using namespace CS2Kit::Players;

namespace
{

CommandResult HandleAdminMenu(Player* admin, const std::vector<std::string>& /*args*/)
{
    if (!admin)
        return {false, "Invalid caller"};

    // Any registered admin may open the menu; each category inside is gated by its own flags.
    if (!App().Admins.IsAdmin(admin->GetSteamID()))
        return {false, "You do not have permission to use this command."};

    int slot = admin->GetSlot();

    // Panel language is registered at connect (see AdminSystemPlugin::OnPlayerConnect).
    auto menu = AdminSystem::Admin::BuildAdminMainMenu(slot);
    if (!menu)
        return {false, "Failed to open admin menu"};

    Engine().Menus.OpenMenu(slot, menu);
    return {true, ""};  // menu UI is the feedback; no chat reply needed
}

}  // namespace

void RegisterAdminMenuCommand(CommandManager& mgr)
{
    mgr.Register(CommandBuilder("admin")
                     .WithAliases({"a", "menu"})
                     .WithDescription("Open the admin menu")
                     .WithUsage("!admin")
                     .WithArgs(0, 0)
                     .OnExecute(HandleAdminMenu)
                     .Build());
}

}  // namespace AdminSystem::Commands
