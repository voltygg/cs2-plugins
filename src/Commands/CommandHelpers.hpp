#pragma once

#include <CS2Kit/Players/Player.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace AdminSystem::Commands::Helpers
{

/**
 * Glue all args from `start` onwards back into a single phrase, defaulting to `fallback`
 * when the caller didn't pass any reason. Lets `!ban Bob 60 he was cheating` keep its words.
 */
std::string JoinReason(const std::vector<std::string>& args, std::size_t start, const std::string& fallback);

/**
 * Resolve a target token down to a single allowed player. Returns nullptr and writes the
 * failure reason into `outError` when ambiguous, missing, or blocked by immunity.
 */
CS2Kit::Players::Player* ResolveSingle(const std::string& token, CS2Kit::Players::Player* caller,
                                       std::string& outError);

/** Parse a duration (in minutes) argument; 0 means permanent. Returns false on bad input. */
bool ParseDurationMinutes(const std::string& arg, int64_t& outSeconds);

/**
 * Fill the common target/admin/reason fields on a punishment entity, plus `Duration`
 * when the entity has one (Ban/VoiceMute/TextMute do; Warning does not). The Ban entity also has
 * TargetIp; callers set that one separately to keep this template simple.
 */
template <typename T>
void FillPunishment(T& punishment, const CS2Kit::Players::Player* target, const CS2Kit::Players::Player* admin,
                    const std::string& reason, int64_t durationSec)
{
    punishment.TargetSteamId = target->GetSteamID();
    punishment.TargetName = target->GetName();
    punishment.AdminSteamId = admin->GetSteamID();
    punishment.AdminName = admin->GetName();
    punishment.Reason = reason;

    if constexpr (requires { punishment.Duration = durationSec; })
    {
        punishment.Duration = durationSec;
    }
}

}  // namespace AdminSystem::Commands::Helpers
