#pragma once

#include <cstddef>

namespace AdminSystem::Maps
{

/**
 * How many yes ballots are needed out of @p cast.
 *
 * A strict majority of the configured share, so a ratio of 0.6 with 10 ballots needs 7. The
 * ratio is clamped, so a mis-set value can make the vote neither free nor impossible.
 *
 * @return 0 when nothing was cast, so an empty vote never sits at an unreachable threshold.
 */
std::size_t VoteThreshold(std::size_t cast, double successRatio);

/** Whether @p yes has reached the threshold for @p cast ballots. */
bool VotePassed(std::size_t yes, std::size_t cast, double successRatio);

}  // namespace AdminSystem::Maps
