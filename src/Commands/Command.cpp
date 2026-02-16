#include "Command.hpp"
#include "../Utils/StringUtils.hpp"

namespace AdminSystem::Commands {

using namespace AdminSystem::Utils;

bool Command::Matches(const std::string& cmd) const
{
    auto lower = StringUtils::ToLower(cmd);
    if (StringUtils::ToLower(Name) == lower)
        return true;
    for (const auto& alias : Aliases)
    {
        if (StringUtils::ToLower(alias) == lower)
            return true;
    }
    return false;
}

} // namespace AdminSystem::Commands
