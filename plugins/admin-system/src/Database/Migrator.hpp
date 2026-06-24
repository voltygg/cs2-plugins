#pragma once

#include <string>

namespace AdminSystem::Database
{

class Database;

/**
 * Apply pending forward-only migrations to `db`. Reads `dir` for files named `NNNN_*.sql` (the leading
 * integer is the version), and applies every file whose version exceeds the max recorded in
 * `schema_migrations`, in ascending order, each in its own transaction, under a session advisory lock
 * so two concurrent plugin loads cannot race. A missing directory is a no-op (logged). Returns false if
 * a migration failed -- the database is left at the last successfully applied version.
 */
bool RunMigrations(Database& db, const std::string& dir);

}  // namespace AdminSystem::Database
