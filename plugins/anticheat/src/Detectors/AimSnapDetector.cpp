#include "AimSnapDetector.hpp"

#include <cmath>
#include <format>

namespace Anticheat::Detectors::AimSnap
{

using CS2Kit::UserCmdView;
namespace AngleMath = CS2Kit::AngleMath;

namespace
{
// Angular step between two consecutive usercmds, including the largest
// single-subtick jump inside the newer command (subtick aim writes are how
// cheats hide flicks from per-tick sampling).
float StepDeg(const UserCmdView& newer, const UserCmdView& older)
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
}  // namespace

std::optional<Detection> OnFire(const AimSnapSettings& cfg, PlayerState& s, int slot)
{
    if (!cfg.enabled)
        return std::nullopt;

    auto& history = CS2Kit::Engine().InputHistory;
    int available = std::min(history.Count(slot), cfg.lookbackTicks);
    if (available < 3)
        return std::nullopt;

    // Find the largest step in the lookback, then require everything between
    // the snap and the shot to be near-still (the "lock" half of snap-and-lock).
    float bestSnap = 0.0f;
    int bestAgo = -1;
    for (int ago = 1; ago < available; ++ago)
    {
        float step = StepDeg(history.At(slot, ago - 1), history.At(slot, ago));
        if (step > bestSnap)
        {
            bestSnap = step;
            bestAgo = ago;
        }
    }
    if (bestSnap < cfg.minSnapDeg)
        return std::nullopt;

    for (int ago = 1; ago < bestAgo; ++ago)
        if (StepDeg(history.At(slot, ago - 1), history.At(slot, ago)) > cfg.settleEpsilonDeg)
            return std::nullopt;

    s.SnapPending = true;
    s.SnapFireTick = s.Tick;

    double now = NowSeconds();
    return Detection{
        .Detector = "aimSnap",
        .ScoreAdd = cfg.snapScore,
        .EventsInWindow = s.SnapHits.CountWithin(now, cfg.eventWindowSec),
        .MinEvents = cfg.minEvents,
        .AlertScore = cfg.alertScore,
        .BanScore = cfg.banScore,
        .DecayPerSec = cfg.decayPerSec,
        .Detail = std::format("unconfirmed {:.0f} deg snap {} ticks before fire", bestSnap, bestAgo),
    };
}

std::optional<Detection> OnDamage(const AimSnapSettings& cfg, PlayerState& s, float aimErrorDeg, double now)
{
    if (!cfg.enabled || !s.SnapPending)
        return std::nullopt;
    if (s.Tick - s.SnapFireTick > static_cast<uint32_t>(cfg.confirmWindowTicks))
    {
        s.SnapPending = false;
        return std::nullopt;
    }
    if (aimErrorDeg > cfg.onTargetEpsilonDeg)
        return std::nullopt;

    s.SnapPending = false;
    s.SnapHits.Push(now);
    int events = s.SnapHits.CountWithin(now, cfg.eventWindowSec);
    return Detection{
        .Detector = "aimSnap",
        .ScoreAdd = cfg.confirmedHitScore,
        .EventsInWindow = events,
        .MinEvents = cfg.minEvents,
        .AlertScore = cfg.alertScore,
        .BanScore = cfg.banScore,
        .DecayPerSec = cfg.decayPerSec,
        .Detail = std::format("snap confirmed by on-target hit, aim error {:.1f} deg ({} in window)", aimErrorDeg,
                              events),
    };
}

}  // namespace Anticheat::Detectors::AimSnap
