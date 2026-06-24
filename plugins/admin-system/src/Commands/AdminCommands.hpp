#pragma once

#include <CS2Kit/Commands/CommandManager.hpp>

namespace AdminSystem::Commands
{

/**
 * Register all admin chat commands (`!kick`, `!ban`, `!unban`, `!voice_mute`,
 * `!voice_unmute`, `!text_mute`, `!text_unmute`, `!warn`, `!who`, `!admin_reload`)
 * on the given command manager. Legacy `!mute`/`!unmute`/`!gag`/`!ungag` remain
 * registered as aliases.
 * Idempotent: safe to call multiple times (later registrations replace earlier ones).
 */
void RegisterAdminCommands(CS2Kit::Commands::CommandManager& mgr);

}  // namespace AdminSystem::Commands
