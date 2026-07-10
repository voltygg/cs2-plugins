#include "SpinbotDetector.hpp"

#include <cmath>
#include <format>

namespace Anticheat::Detectors::Spinbot
{

float SpinYawDelta(const CS2Kit::Sdk::UserCmdView& cmd, float normalizedFallback)
{
    if (cmd.SubtickMoveCount <= 0)
        return normalizedFallback;
    float sum = 0.0f;
    for (int i = 0; i < cmd.SubtickMoveCount; ++i)
        sum += cmd.SubtickMoves[i].YawDelta;
    return sum;
}

std::optional<Detection> OnCmd(const SpinSettings& cfg, PlayerState& s, float yawDelta)
{
    if (!cfg.enabled)
        return std::nullopt;

    float magnitude = std::fabs(yawDelta);
    if (s.YawDeltaCount == PlayerState::SpinWindow)
        s.YawDeltaSum -= s.YawDeltas[s.YawDeltaHead];
    else
        ++s.YawDeltaCount;
    s.YawDeltas[s.YawDeltaHead] = magnitude;
    s.YawDeltaHead = (s.YawDeltaHead + 1) % PlayerState::SpinWindow;
    s.YawDeltaSum += magnitude;

    float degPerSec = (s.YawDeltaSum / static_cast<float>(s.YawDeltaCount)) * TickRate;
    bool spinningNow = s.YawDeltaCount == PlayerState::SpinWindow && degPerSec >= cfg.yawVelocityDegPerSec;
    if (!spinningNow)
    {
        s.SpinTicks = 0;
        return std::nullopt;
    }

    ++s.SpinTicks;
    if (s.SpinTicks < static_cast<uint32_t>(cfg.minTicks))
        return std::nullopt;

    s.LastSpinTick = s.Tick;

    // Observe-only note when a spin first arms, throttled so a minute of fake-spinning doesn't
    // flood the log. Its own bucket keeps it out of the kill-based "spin" score (legit players
    // fake-spin for fun, so spinning alone must never escalate).
    double now = NowSeconds();
    if (s.SpinTicks == static_cast<uint32_t>(cfg.minTicks) && now - s.LastSpinOnlyReport > 10.0)
    {
        s.LastSpinOnlyReport = now;
        return Detection{
            .Detector = "spin.idle",
            .ScoreAdd = cfg.spinOnlyScore,
            .DecayPerSec = cfg.decayPerSec,
            .ObserveOnly = true,
            .Detail = std::format("spinning at {:.0f} deg/s (no kill correlation)", degPerSec),
        };
    }
    return std::nullopt;
}

bool RecentlySpinning(const SpinSettings& cfg, const PlayerState& s)
{
    return s.LastSpinTick != 0 && s.Tick - s.LastSpinTick <= static_cast<uint32_t>(cfg.killWindowTicks);
}

std::optional<Detection> OnKill(const SpinSettings& cfg, PlayerState& s, bool headshot, float aimErrorDeg, double now)
{
    if (!cfg.enabled || !RecentlySpinning(cfg, s))
        return std::nullopt;
    if (aimErrorDeg > cfg.onTargetEpsilonDeg)
        return std::nullopt;

    s.SpinKills.Push(now);
    int events = s.SpinKills.CountWithin(now, cfg.eventWindowSec);
    float score = cfg.killEventScore * (headshot ? cfg.headshotMultiplier : 1.0f);
    return Detection{
        .Detector = "spin",
        .ScoreAdd = score,
        .EventsInWindow = events,
        .MinEvents = cfg.minKillEvents,
        .AlertScore = cfg.alertScore,
        .BanScore = cfg.banScore,
        .DecayPerSec = cfg.decayPerSec,
        .Detail = std::format("mid-spin {} kill, aim error {:.1f} deg ({} spin-kills in window)",
                              headshot ? "headshot" : "on-target", aimErrorDeg, events),
    };
}

}  // namespace Anticheat::Detectors::Spinbot
