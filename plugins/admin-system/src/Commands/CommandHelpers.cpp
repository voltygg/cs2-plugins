#include "CommandHelpers.hpp"

#include "TargetResolver.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <utility>

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

std::string CallerText(const Player* caller, const std::string& key, const std::map<std::string, std::string>& tokens)
{
    auto& tr = CS2Kit::Core::Engine().Translations;
    std::string text = caller ? tr.Get(key, caller->GetSlot()) : tr.Get(key);
    return StringUtils::SubstituteTokens(std::move(text), tokens);
}

Player* ResolveSingle(const std::string& token, Player* caller, std::string& outError)
{
    auto matches = Resolve(token, caller);
    if (matches.empty())
    {
        outError = CallerText(caller, "target.noMatch", {{"token", token}});
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
        outError = CallerText(caller, "target.immune", {{"token", token}});
        return nullptr;
    }
    if (allowed.size() > 1)
    {
        outError = CallerText(caller, "target.ambiguous", {{"token", token}, {"count", std::to_string(allowed.size())}});
        return nullptr;
    }
    return allowed[0];
}

bool ParseCommandDuration(const std::string& arg, int64_t& outSeconds)
{
    int seconds = ParseDuration(arg);
    if (seconds < 0)
    {
        return false;
    }
    // A bare number keeps the legacy command meaning (minutes); ParseDuration read it as seconds.
    outSeconds = StringUtils::IsNumeric(arg) ? static_cast<int64_t>(seconds) * 60 : seconds;
    return true;
}

}  // namespace AdminSystem::Commands::Helpers
