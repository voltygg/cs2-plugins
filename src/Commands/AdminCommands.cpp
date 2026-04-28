#include "AdminCommands.hpp"

#include "InfoCommands.hpp"
#include "PunishmentCommands.hpp"

namespace AdminSystem::Commands
{

void RegisterAdminCommands(CS2Kit::Commands::CommandManager& mgr)
{
    RegisterPunishmentCommands(mgr);
    RegisterInfoCommands(mgr);
}

}  // namespace AdminSystem::Commands
