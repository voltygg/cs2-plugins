#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace AdminSystem::Config
{

/** Server identity in the shared database.
 *  The tag keys grants and audit records, so it must remain unique and stable. */
struct ServerSettings
{
    std::string tag = "default";
    std::string name = "CS2 Server";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ServerSettings, tag, name)

}  // namespace AdminSystem::Config
