#pragma once

#include <CS2Kit/Players/Player.hpp>
#include <string>
#include <vector>

namespace AdminSystem::Commands
{

/** Outcome of attempting to resolve a single target token (e.g. "Bob", "#3"). */
struct ResolvedTarget
{
    CS2Kit::Players::Player* Player = nullptr;  /**< null when unresolved. */
    bool Allowed = true;                        /**< false when blocked by immunity. */
};

/**
 * Resolve a target token to a list of online players.
 *
 * Supported syntax (no SourceMod-style group selectors yet):
 *   - `#N` — slot index N (e.g. "#3").
 *   - anything else — case-insensitive substring of the player's display name.
 *
 * Caller's immunity is checked against each match; blocked entries appear with
 * `Allowed = false` so the command handler can report "X is immune".
 * Returns empty when nothing matched at all (caller should report "no match").
 */
std::vector<ResolvedTarget> Resolve(const std::string& token, CS2Kit::Players::Player* caller);

}  // namespace AdminSystem::Commands
