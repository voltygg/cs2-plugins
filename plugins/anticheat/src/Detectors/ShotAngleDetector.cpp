#include "ShotAngleDetector.hpp"

#include <CS2Kit/Utils/AngleMath.hpp>
#include <algorithm>
#include <format>

namespace Anticheat::Detectors::ShotAngle
{

namespace AngleMath = CS2Kit::Utils::AngleMath;

std::optional<float> ShotDivergence(const CS2Kit::Sdk::UserCmdView& cmd)
{
    if (cmd.Attack1StartHistoryIndex < 0 || cmd.InputHistorySampleCount <= 0)
        return std::nullopt;

    // The index addresses the full input_history; the decode kept only the first MaxInputHistory.
    int idx = std::min(cmd.Attack1StartHistoryIndex, cmd.InputHistorySampleCount - 1);
    const auto& shot = cmd.InputHistorySamples[idx];
    if (!shot.HasViewAngles)
        return std::nullopt;

    return AngleMath::AngularDistance({.Pitch = cmd.ViewPitch, .Yaw = cmd.ViewYaw},
                                      {.Pitch = shot.ViewPitch, .Yaw = shot.ViewYaw});
}

void OnCmd(const ShotAngleSettings& cfg, PlayerState& s, const CS2Kit::Sdk::UserCmdView& cmd)
{
    if (!cfg.enabled)
        return;

    auto diverge = ShotDivergence(cmd);
    if (!diverge || *diverge < cfg.minDivergenceDeg)
        return;

    s.ShotDivergePending = true;
    s.ShotDivergeTick = s.Tick;
    s.ShotDivergeDeg = *diverge;
}

std::optional<Detection> OnDamage(const ShotAngleSettings& cfg, PlayerState& s, double now)
{
    if (!cfg.enabled || !s.ShotDivergePending)
        return std::nullopt;
    if (s.Tick - s.ShotDivergeTick > static_cast<uint32_t>(cfg.confirmWindowTicks))
    {
        s.ShotDivergePending = false;
        return std::nullopt;
    }

    s.ShotDivergePending = false;
    s.ShotDivergeHits.Push(now);
    int events = s.ShotDivergeHits.CountWithin(now, cfg.eventWindowSec);
    return Detection{
        .Detector = "shotAngle",
        .ScoreAdd = cfg.eventScore,
        .EventsInWindow = events,
        .MinEvents = cfg.minEvents,
        .AlertScore = cfg.alertScore,
        .BanScore = cfg.banScore,
        .DecayPerSec = cfg.decayPerSec,
        .Detail = std::format("shot angle diverged {:.1f} deg from view and connected ({} in window)", s.ShotDivergeDeg,
                              events),
    };
}

}  // namespace Anticheat::Detectors::ShotAngle
