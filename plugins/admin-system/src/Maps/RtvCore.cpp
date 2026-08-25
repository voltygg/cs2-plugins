#include "RtvCore.hpp"

#include <algorithm>
#include <cmath>

namespace AdminSystem::Maps
{

std::size_t RtvThreshold(std::size_t humans, double successRatio)
{
    if (humans == 0)
        return 0;

    // Clamped so a mis-set ratio cannot make the vote either free or impossible.
    double ratio = std::clamp(successRatio, 0.0, 1.0);
    auto needed = static_cast<std::size_t>(static_cast<double>(humans) * ratio) + 1;
    return std::min(needed, humans);
}

bool RtvPassed(std::size_t votes, std::size_t humans, double successRatio)
{
    std::size_t needed = RtvThreshold(humans, successRatio);
    return needed > 0 && votes >= needed;
}

}  // namespace AdminSystem::Maps
