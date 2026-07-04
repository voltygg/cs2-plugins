#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Commands/CommandManager.hpp>

namespace AdminSystem::Commands
{

/** Register the `!admin` (aliases: `!a`, `!menu`) command that opens the top-level admin menu. */
void RegisterAdminMenuCommand(CS2Kit::CommandManager& mgr);

}  // namespace AdminSystem::Commands
