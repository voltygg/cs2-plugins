#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Commands/CommandManager.hpp>

namespace AdminSystem::Commands
{

/** Register self-targeted admin commands (`!hide`). */
void RegisterAdminSelfCommands(CS2Kit::CommandManager& mgr);

}  // namespace AdminSystem::Commands
