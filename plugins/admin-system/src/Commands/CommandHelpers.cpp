#include "CommandHelpers.hpp"

#include "TargetResolver.hpp"

#include <CS2Kit/Utils/StringUtils.hpp>

#include <format>

namespace AdminSystem::Commands::Helpers
{

using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;

std::string JoinReason(const std::vector<std::string>& args, std::size_t start, const std::string& fallback)
{
    if (start >= args.size())
    {
        return fallback;
    }
    std::vector<std::string> rest(args.begin() + start, args.end());
    return StringUtils::Join(rest, " ");
}

Player* ResolveSingle(const std::string& token, Player* caller, std::string& outError)
{
    auto matches = Resolve(token, caller);
    if (matches.empty())
    {
        outError = std::format("No player matched '{}'.", token);
        return nullptr;
    }

    // Prefer allowed targets; if all matches are blocked by immunity, fail with that reason.
    std::vector<Player*> allowed;
    for (const auto& m : matches)
    {
        if (m.Allowed && m.Player)
        {
            allowed.push_back(m.Player);
        }
    }

    if (allowed.empty())
    {
        outError = std::format("Target '{}' is immune.", token);
        return nullptr;
    }
    if (allowed.size() > 1)
    {
        outError = std::format("'{}' matches {} players -- be more specific.", token, allowed.size());
        return nullptr;
    }
    return allowed[0];
}

bool ParseDurationMinutes(const std::string& arg, int64_t& outSeconds)
{
    if (!StringUtils::IsNumeric(arg))
    {
        return false;
    }
    int64_t mins = std::stoll(arg);
    if (mins < 0)
    {
        return false;
    }
    outSeconds = mins * 60;
    return true;
}

}  // namespace AdminSystem::Commands::Helpers
