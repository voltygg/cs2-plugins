#pragma once

namespace AdminSystem::Core::Bootstrap
{

/**
 * Wire up admin-system subsystems after CS2Kit has been initialized:
 * load configs/translations, connect to PostgreSQL, install command callbacks,
 * register chat commands, schedule the punishment-expiry sweep, and subscribe to
 * effect-cancelling game events.
 *
 * Returns false on a fatal config error. Database failures are non-fatal: the plugin
 * stays loaded with admins/punishments disabled (only console can issue commands).
 */
bool Initialize();

/** Tear down admin-system state owned by the plugin (effects, players, DB connection). */
void Shutdown();

}  // namespace AdminSystem::Core::Bootstrap
