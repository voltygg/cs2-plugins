#pragma once

#include <VoltMod/Database/PostgresDatabase.hpp>
#include <nlohmann/json.hpp>

namespace VoltMod
{
// ADL requires this mapper in PostgresConfig's namespace. Keeping it here also
// keeps nlohmann out of the framework header.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PostgresConfig, host, port, database, username, password, sslMode)
}  // namespace VoltMod

namespace AdminSystem::Config
{

using DatabaseConfig = VoltMod::PostgresConfig;

}  // namespace AdminSystem::Config
