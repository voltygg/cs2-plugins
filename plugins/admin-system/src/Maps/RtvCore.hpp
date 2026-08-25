#pragma once

#include <cstddef>

namespace AdminSystem::Maps
{

/**
 * How many RTV votes are needed out of @p humans players.
 *
 * Mirrors the ratio rule most servers expect: a strict majority of the configured share, so a
 * ratio of 0.6 with 10 players needs 7. Bots are excluded by the caller, since they never vote
 * and would otherwise raise the bar out of reach on a mostly-empty server.
 *
 * @return 0 when nobody is connected, so an empty server never sits at an unreachable threshold.
 */
std::size_t RtvThreshold(std::size_t humans, double successRatio);

/** Whether @p votes has reached the threshold for @p humans players. */
bool RtvPassed(std::size_t votes, std::size_t humans, double successRatio);

}  // namespace AdminSystem::Maps
