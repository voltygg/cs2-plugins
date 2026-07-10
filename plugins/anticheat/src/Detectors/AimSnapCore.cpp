#include "AimSnapCore.hpp"

#include <CS2Kit/Utils/AngleMath.hpp>
#include <algorithm>
#include <cmath>

namespace Anticheat::Detectors::AimSnap
{

namespace AngleMath = CS2Kit::Utils::AngleMath;

float StepDeg(const CS2Kit::Sdk::UserCmdView& newer, const CS2Kit::Sdk::UserCmdView& older)
{
    float step = AngleMath::AngularDistance({.Pitch = newer.ViewPitch, .Yaw = newer.ViewYaw},
                                            {.Pitch = older.ViewPitch, .Yaw = older.ViewYaw});

    for (int i = 0; i < newer.SubtickMoveCount; ++i)
    {
        const auto& move = newer.SubtickMoves[i];
        float sub = std::sqrt(move.YawDelta * move.YawDelta + move.PitchDelta * move.PitchDelta);
        step = std::max(step, sub);
    }
    return step;
}

}  // namespace Anticheat::Detectors::AimSnap
