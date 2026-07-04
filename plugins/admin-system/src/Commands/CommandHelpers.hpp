#pragma once

#include <CS2Kit/Api.hpp>
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
CS2Kit::Player* ResolveSingle(const std::string& token, CS2Kit::Player* caller,
                                       std::string& outError);

/**
 * Parse a command duration argument. A bare number is MINUTES (the legacy command grammar);
 * suffixed forms use the CS2Kit grammar (30s/5m/2h/7d). `0` and `perm`/`permanent` mean
 * permanent under either reading. Returns false on bad input.
 */
bool ParseCommandDuration(const std::string& arg, int64_t& outSeconds);

}  // namespace AdminSystem::Commands::Helpers
