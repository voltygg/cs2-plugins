#include "NoFlashDetector.hpp"

#include <algorithm>
#include <format>

namespace Anticheat::Detectors::NoFlash
{

void OnBlind(const NoFlashSettings& cfg, PlayerState& s, float blindDuration, double now)
{
    if (!cfg.enabled)
        return;
    s.BlindUntil = std::max(s.BlindUntil, now + blindDuration);
}

std::optional<Detection> OnKill(const NoFlashSettings& cfg, PlayerState& s, bool headshot, double now)
{
    if (!cfg.enabled)
        return std::nullopt;

    double remaining = s.BlindUntil - now;
    if (remaining < cfg.minRemainingSec)
        return std::nullopt;

    s.BlindKills.Push(now);
    int events = s.BlindKills.CountWithin(now, cfg.eventWindowSec);
    return Detection{
        .Detector = "noFlash",
        .ScoreAdd = cfg.eventScore,
        .EventsInWindow = events,
        .MinEvents = cfg.minEvents,
        .AlertScore = cfg.alertScore,
        .BanScore = cfg.banScore,
        .DecayPerSec = cfg.decayPerSec,
        .Detail = std::format("{} kill while blinded ({:.1f}s of flash remaining, {} in window)",
                              headshot ? "headshot" : "on-target", remaining, events),
    };
}

}  // namespace Anticheat::Detectors::NoFlash
