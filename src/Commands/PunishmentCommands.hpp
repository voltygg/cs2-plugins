#pragma once

#include <CS2Kit/Commands/CommandManager.hpp>

namespace AdminSystem::Commands
{

/** Register `!kick`, `!ban`, `!unban`, `!mute`, `!unmute`, `!gag`, `!ungag`, `!warn`. */
void RegisterPunishmentCommands(CS2Kit::Commands::CommandManager& mgr);

}  // namespace AdminSystem::Commands
