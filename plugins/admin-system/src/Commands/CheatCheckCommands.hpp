#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Commands/CommandManager.hpp>

namespace AdminSystem::Commands
{

/** Register public `!cc <link>` (suspect submits a link) and admin `!cccancel <target>`. */
void RegisterCheatCheckCommands(CS2Kit::CommandManager& mgr);

}  // namespace AdminSystem::Commands
