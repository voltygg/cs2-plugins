#include "AimSnapDetector.hpp"

#include "AimSnapCore.hpp"

#include <CS2Kit/Api.hpp>
#include <algorithm>
#include <format>

namespace Anticheat::Detectors::AimSnap
{

std::optional<Detection> OnFire(const AimSnapSettings& cfg, PlayerState& s, int slot)
{
    if (!cfg.enabled)
        return std::nullopt;

    auto& history = CS2Kit::Engine().InputHistory;
    int available = std::min(history.Count(slot), cfg.lookbackTicks);

    auto snap =
        FindSettledSnap([&](int ago) -> const CS2Kit::UserCmdView& { return history.At(slot, ago); }, available, cfg);
    if (!snap)
        return std::nullopt;

    s.SnapPending = true;
    s.SnapFireTick = s.Tick;

    // Unconfirmed: own observe-only bucket so it never escalates (legit flicks look like snaps;
    // only on-target damage confirms one, under the "aimSnap" bucket).
    return Detection{
        .Detector = "aimSnap.idle",
        .ScoreAdd = cfg.snapScore,
        .DecayPerSec = cfg.decayPerSec,
        .ObserveOnly = true,
        .Detail = std::format("unconfirmed {:.0f} deg snap {} ticks before fire", snap->SnapDeg, snap->Ago),
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
        .Detail =
            std::format("snap confirmed by on-target hit, aim error {:.1f} deg ({} in window)", aimErrorDeg, events),
    };
}

}  // namespace Anticheat::Detectors::AimSnap
