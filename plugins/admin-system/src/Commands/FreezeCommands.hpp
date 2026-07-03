#pragma once

#include <CS2Kit/Commands/CommandManager.hpp>

namespace AdminSystem::Commands
{

/** Register the admin-abuse protection commands: !freeze_admin, !unfreeze_admin, !frozen_admins. */
void RegisterFreezeCommands(CS2Kit::Commands::CommandManager& mgr);

}  // namespace AdminSystem::Commands
