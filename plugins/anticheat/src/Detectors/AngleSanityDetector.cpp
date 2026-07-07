#include "AngleSanityDetector.hpp"

#include <cmath>
#include <format>

namespace Anticheat::Detectors::AngleSanity
{

std::optional<Detection> OnCmd(const SanitySettings& cfg, PlayerState& s, float pitch, float yaw)
{
    if (!cfg.enabled)
        return std::nullopt;

    bool bad = !std::isfinite(pitch) || !std::isfinite(yaw) || std::fabs(pitch) > cfg.maxPitchDeg;
    if (!bad)
    {
        s.BadAngleTicks = 0;
        return std::nullopt;
    }

    // Report exactly once per bad streak, at the point it becomes sustained.
    if (++s.BadAngleTicks != cfg.minTicks)
        return std::nullopt;

    double now = NowSeconds();
    s.SanityEvents.Push(now);
    int events = s.SanityEvents.CountWithin(now, cfg.eventWindowSec);
    return Detection{
        .Detector = "sanity",
        .ScoreAdd = cfg.eventScore,
        .EventsInWindow = events,
        .MinEvents = cfg.minEvents,
        .AlertScore = cfg.alertScore,
        .BanScore = cfg.banScore,
        .DecayPerSec = cfg.decayPerSec,
        .Detail = std::format("impossible view angles pitch={} yaw={} for {} ticks", pitch, yaw, cfg.minTicks),
    };
}

}  // namespace Anticheat::Detectors::AngleSanity
