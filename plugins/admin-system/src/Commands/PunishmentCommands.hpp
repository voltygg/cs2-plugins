#pragma once

#include <CS2Kit/Commands/CommandManager.hpp>

namespace AdminSystem::Commands
{

/**
 * Register `!kick`, `!ban`, `!unban`, `!voice_mute`, `!voice_unmute`, `!text_mute`,
 * `!text_unmute`, `!warn`. Legacy `!mute`/`!unmute`/`!gag`/`!ungag` remain as aliases.
 */
void RegisterPunishmentCommands(CS2Kit::Commands::CommandManager& mgr);

}  // namespace AdminSystem::Commands
