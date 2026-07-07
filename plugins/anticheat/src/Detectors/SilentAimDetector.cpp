#include "SilentAimDetector.hpp"

#include <cmath>
#include <cstdlib>
#include <format>

namespace Anticheat::Detectors::SilentAim
{

void OnCmd(const SilentAimSettings& cfg, PlayerState& s, const CS2Kit::UserCmdView& cmd, float yawDelta,
           float pitchDelta)
{
    if (!cfg.enabled)
        return;

    float jump = std::sqrt(yawDelta * yawDelta + pitchDelta * pitchDelta);
    for (int i = 0; i < cmd.SubtickMoveCount; ++i)
    {
        const auto& move = cmd.SubtickMoves[i];
        jump = std::max(jump, std::sqrt(move.YawDelta * move.YawDelta + move.PitchDelta * move.PitchDelta));
    }

    int mouseUnits = std::abs(cmd.MouseDx) + std::abs(cmd.MouseDy);
    if (jump >= cfg.minAngleJumpDeg && mouseUnits <= cfg.maxMouseUnits)
    {
        s.SilentPending = true;
        s.SilentTick = s.Tick;
    }
}

std::optional<Detection> OnDamage(const SilentAimSettings& cfg, PlayerState& s, float aimErrorDeg, double now)
{
    if (!cfg.enabled || !s.SilentPending)
        return std::nullopt;
    if (s.Tick - s.SilentTick > static_cast<uint32_t>(cfg.confirmWindowTicks))
    {
        s.SilentPending = false;
        return std::nullopt;
    }
    if (aimErrorDeg > cfg.onTargetEpsilonDeg)
        return std::nullopt;

    s.SilentPending = false;
    s.SilentHits.Push(now);
    int events = s.SilentHits.CountWithin(now, cfg.eventWindowSec);
    return Detection{
        .Detector = "silentAim",
        .ScoreAdd = cfg.eventScore,
        .EventsInWindow = events,
        .MinEvents = cfg.minEvents,
        .AlertScore = cfg.alertScore,
        .BanScore = cfg.banScore,
        .DecayPerSec = cfg.decayPerSec,
        .Detail = std::format("angle jump with no mouse input landed on-target, aim error {:.1f} deg ({} in window)",
                              aimErrorDeg, events),
    };
}

}  // namespace Anticheat::Detectors::SilentAim
