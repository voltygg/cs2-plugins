#pragma once

#include <CS2Kit/Commands/CommandManager.hpp>

namespace AdminSystem::Commands
{

/**
 * Register all admin chat commands (`!kick`, `!ban`, `!unban`, `!mute`, `!unmute`,
 * `!gag`, `!ungag`, `!warn`, `!who`, `!admin_reload`) on the given command manager.
 * Idempotent: safe to call multiple times (later registrations replace earlier ones).
 */
void RegisterAdminCommands(CS2Kit::Commands::CommandManager& mgr);

}  // namespace AdminSystem::Commands
