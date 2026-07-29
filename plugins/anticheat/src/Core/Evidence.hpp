#pragma once

// The rolling "N incidents within the window" counter every core thresholds on. The window is a
// template parameter so a default-constructed core - and every `= {}` reset - carries it.

#include <CS2Kit/Utils/SlidingWindowScore.hpp>

namespace Anticheat
{

template <int WindowSec>
struct EvidenceWindow : CS2Kit::Utils::SlidingWindowScore
{
    EvidenceWindow() : SlidingWindowScore(WindowSec) {}
};

/** Aim modules judge over ten minutes: long enough that a cheat cannot wait a detection out. */
using LongEvidenceWindow = EvidenceWindow<600>;

}  // namespace Anticheat
