#pragma once

#include <string>

namespace Anticheat
{

/** Assumed server tick rate for deg/sec conversions (CS2 is 64-tick). */
inline constexpr float TickRate = 64.0f;

/**
 * One detector finding, routed to the ResponseManager. EventsInWindow vs
 * MinEvents is the structural false-positive guard: the ban tier requires both
 * the score threshold AND enough distinct confirmed events inside the window.
 */
struct Detection
{
    const char* Detector = "";
    float ScoreAdd = 0.0f;
    int EventsInWindow = 0;
    int MinEvents = 1;
    float AlertScore = 0.0f;
    float BanScore = 0.0f;
    float DecayPerSec = 0.0f;
    std::string Detail;
};

}  // namespace Anticheat
