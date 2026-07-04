#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Commands/CommandManager.hpp>

namespace AdminSystem::Commands
{

/** Register `!who` (alias `!players`) and `!admin_reload` (alias `!reload_admins`). */
void RegisterInfoCommands(CS2Kit::CommandManager& mgr);

}  // namespace AdminSystem::Commands
