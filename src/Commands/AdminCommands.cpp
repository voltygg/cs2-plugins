#include "AdminCommands.hpp"

#include "AdminSelfCommands.hpp"
#include "InfoCommands.hpp"
#include "PunishmentCommands.hpp"

namespace AdminSystem::Commands
{

void RegisterAdminCommands(CS2Kit::Commands::CommandManager& mgr)
{
    RegisterPunishmentCommands(mgr);
    RegisterInfoCommands(mgr);
    RegisterAdminSelfCommands(mgr);
}

}  // namespace AdminSystem::Commands
