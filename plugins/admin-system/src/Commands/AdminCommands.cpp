#include "AdminCommands.hpp"

#include "AdminMenuCommand.hpp"
#include "AdminSelfCommands.hpp"
#include "CheatCheckCommands.hpp"
#include "FreezeCommands.hpp"
#include "InfoCommands.hpp"
#include "PunishmentCommands.hpp"

#include <CS2Kit/Api.hpp>

namespace AdminSystem::Commands
{

void RegisterAdminCommands(CS2Kit::CommandManager& mgr)
{
    RegisterAdminMenuCommand(mgr);
    RegisterPunishmentCommands(mgr);
    RegisterInfoCommands(mgr);
    RegisterAdminSelfCommands(mgr);
    RegisterCheatCheckCommands(mgr);
    RegisterFreezeCommands(mgr);
}

}  // namespace AdminSystem::Commands
