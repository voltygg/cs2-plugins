#include "AdminMenuCommand.hpp"
#include "../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

#include "../Admin/AdminManager.hpp"
#include "../Admin/AdminMenu.hpp"

#include <CS2Kit/Menu/MenuManager.hpp>

using CS2Kit::Core::Kit;

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

    int slot = admin->GetSlot();

    // Panel language is registered at connect (see AdminSystemPlugin::OnPlayerConnect).
    auto menu = AdminSystem::Admin::BuildAdminMainMenu(slot);
    if (!menu)
        return {false, "Failed to open admin menu"};

    Kit().Menus.OpenMenu(slot, menu);
    return {true, ""};  // menu UI is the feedback; no chat reply needed
}

}  // namespace

void RegisterAdminMenuCommand(CommandManager& mgr)
{
    mgr.Register(CommandBuilder("admin")
                     .WithAliases({"a", "menu"})
                     .WithDescription("Open the admin menu")
                     .WithUsage("!admin")
                     .RequirePermission("r")
                     .WithArgs(0, 0)
                     .OnExecute(HandleAdminMenu)
                     .Build());
}

}  // namespace AdminSystem::Commands
