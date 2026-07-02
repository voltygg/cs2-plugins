#pragma once

#include <CS2Kit/Commands/CommandManager.hpp>

namespace AdminSystem::Commands
{

/**
 * Register `!kick`, `!ban`, `!unban`, `!voice_mute`, `!voice_unmute`, `!text_mute`,
 * `!text_unmute`, `!warn`. SourceMod-style aliases are registered too:
 * `!mute`/`!unmute` map to voice mute, `!gag`/`!ungag` to text mute.
 */
void RegisterPunishmentCommands(CS2Kit::Commands::CommandManager& mgr);

}  // namespace AdminSystem::Commands
