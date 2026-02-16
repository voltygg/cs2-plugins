#include "Command.hpp"
#include "../Utils/StringUtils.hpp"

namespace AdminSystem::Commands {

bool Command::Matches(const std::string& cmd) const
{
    auto lower = Utils::StringUtils::ToLower(cmd);
    if (Utils::StringUtils::ToLower(Name) == lower)
        return true;
    for (const auto& alias : Aliases)
    {
        if (Utils::StringUtils::ToLower(alias) == lower)
            return true;
    }
    return false;
}

} // namespace AdminSystem::Commands
