#include "AdminCommands.hpp"

#include "AdminMenuCommand.hpp"
#include "AdminSelfCommands.hpp"
#include "CheatCheckCommands.hpp"
#include "FreezeCommands.hpp"
#include "InfoCommands.hpp"
#include "PunishmentCommands.hpp"

namespace AdminSystem::Commands
{

void RegisterAdminCommands(CS2Kit::Commands::CommandManager& mgr)
{
    RegisterAdminMenuCommand(mgr);
    RegisterPunishmentCommands(mgr);
    RegisterInfoCommands(mgr);
    RegisterAdminSelfCommands(mgr);
    RegisterCheatCheckCommands(mgr);
    RegisterFreezeCommands(mgr);
}

}  // namespace AdminSystem::Commands
