#include "VoteMath.hpp"

#include <algorithm>

namespace AdminSystem::Maps
{

std::size_t VoteThreshold(std::size_t cast, double successRatio)
{
    if (cast == 0)
        return 0;

    // Clamped so a mis-set ratio cannot make the vote either free or impossible.
    double ratio = std::clamp(successRatio, 0.0, 1.0);
    auto needed = static_cast<std::size_t>(static_cast<double>(cast) * ratio) + 1;
    return std::min(needed, cast);
}

bool VotePassed(std::size_t yes, std::size_t cast, double successRatio)
{
    std::size_t needed = VoteThreshold(cast, successRatio);
    return needed > 0 && yes >= needed;
}

}  // namespace AdminSystem::Maps
