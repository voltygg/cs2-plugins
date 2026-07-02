#include "CommandHelpers.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Core/Managers.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Players/TargetResolver.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <utility>

namespace AdminSystem::Commands::Helpers
{

using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;

namespace
{
/** Immunity policy shared by every command target resolution. */
bool CanTarget(Player& caller, Player& target)
{
    return App().Admins.CanTarget(caller.GetSteamID(), target.GetSteamID());
}
}  // namespace

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
    auto& tr = CS2Kit::Core::Engine().Translations;
    int callerSlot = caller ? caller->GetSlot() : -1;  // -1 = server language (console callers)

    auto result = ResolveSingleTarget(token, caller, CanTarget);
    switch (result.Error)
    {
    case SingleTargetError::NoMatch:
        outError = tr.Get("target.noMatch", callerSlot, {{"token", token}});
        return nullptr;
    case SingleTargetError::Immune:
        outError = tr.Get("target.immune", callerSlot, {{"token", token}});
        return nullptr;
    case SingleTargetError::Ambiguous:
        outError =
            tr.Get("target.ambiguous", callerSlot, {{"token", token}, {"count", std::to_string(result.MatchCount)}});
        return nullptr;
    case SingleTargetError::None:
        break;
    }
    return result.Target;
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
