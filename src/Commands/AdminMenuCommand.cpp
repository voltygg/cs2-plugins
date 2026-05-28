#include "AdminMenuCommand.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Admin/AdminMenu.hpp"

#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Commands;
using namespace CS2Kit::Menu;
using namespace CS2Kit::Players;
using CS2Kit::Utils::Translations;

namespace
{

CommandResult HandleAdminMenu(Player* admin, const std::vector<std::string>& /*args*/)
{
    if (!admin)
        return {false, "Invalid caller"};

    int slot = admin->GetSlot();

    // Register this admin's panel language so the menu builds and renders in it.
    const auto* row = AdminSystem::Admin::AdminManager::Instance().GetAdmin(admin->GetSteamID());
    Translations::Instance().SetPlayerLanguage(slot, row ? row->Language : std::string("en"));

    Translations::SlotScope langScope(slot);
    auto menu = AdminSystem::Admin::BuildAdminMainMenu(slot);
    if (!menu)
        return {false, "Failed to open admin menu"};

    MenuManager::Instance().OpenMenu(slot, menu);
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
