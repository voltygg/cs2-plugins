#pragma once

#include <VoltMod/Core/SlidingWindowScore.hpp>

namespace Anticheat
{

template <int WindowSec>
struct EvidenceWindow : VoltMod::SlidingWindowScore
{
    EvidenceWindow() : SlidingWindowScore(WindowSec) {}
};

/** Aim modules judge over ten minutes: long enough that a cheat cannot wait a detection out. */
using LongEvidenceWindow = EvidenceWindow<600>;

}  // namespace Anticheat
