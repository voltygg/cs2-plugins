#pragma once

#include <CS2Kit/Database/Migrator.hpp>
#include <CS2Kit/Sdk/ServerCommand.hpp>

namespace AdminSystem::Core
{

/**
 * `admin_status` server-console command: human-readable health report, or one
 * machine-readable `STATUS_JSON {...}` line with the `json` argument (parseable
 * over RCON, e.g. `poe rcon "admin_status json"`).
 */
class StatusCommand
{
public:
    StatusCommand();

    /** Load-time migration outcome, surfaced in the db section. Set by OnLoad. */
    CS2Kit::Database::MigrationResult Migration;

private:
    CS2Kit::Sdk::ServerCommand _command;
};

}  // namespace AdminSystem::Core
