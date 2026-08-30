#pragma once

#include <VoltMod/Database/PostgresConfig.hpp>

namespace AdminSystem::Config
{

// PostgresConfig's fields are lowercase so a JSON section maps straight onto them, and reflection
// needs no mapper - the "database" section deserializes into it as it stands. Note that this
// exposes every field, `connectTimeoutSec` included.
using DatabaseConfig = VoltMod::PostgresConfig;

}  // namespace AdminSystem::Config
