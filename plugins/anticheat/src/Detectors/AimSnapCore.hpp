#pragma once

#include "../Config.hpp"

#include <CS2Kit/Sdk/UserCmd.hpp>
#include <optional>
#include <span>

namespace Anticheat::Detectors::AimSnap
{

struct SnapCandidate
{
    float SnapDeg = 0.0f;
    int Ago = 0;  // ticks before the shot
};

/** Angular step between two usercmds, taking the largest subtick jump (cheats hide flicks in subticks). */
float StepDeg(const CS2Kit::Sdk::UserCmdView& newer, const CS2Kit::Sdk::UserCmdView& older);

/**
 * Snap-and-lock over a newest-first window addressed through @p at, where at(0) is the fire-tick
 * command. The snap is the FIRST step exceeding settleEpsilonDeg walking back from the shot, not
 * the global max, so a larger earlier legit motion can't mask the aimbot's snap right before the
 * fire. Returned only when that step is at least minSnapDeg.
 *
 * Taking an accessor rather than an array keeps this a pure function (testable without the Engine)
 * while letting the live path read the input-history ring in place - UserCmdView is ~550 bytes.
 */
template <typename At>
std::optional<SnapCandidate> FindSettledSnap(At&& at, int count, const AimSnapSettings& cfg)
{
    if (count < 3)
        return std::nullopt;

    for (int ago = 1; ago < count; ++ago)
    {
        float step = StepDeg(at(ago - 1), at(ago));
        if (step <= cfg.settleEpsilonDeg)
            continue;  // still in the lock
        if (step < cfg.minSnapDeg)
            return std::nullopt;  // motion, but not a snap-sized flick
        return SnapCandidate{.SnapDeg = step, .Ago = ago};
    }
    return std::nullopt;
}

/** Contiguous newest-first window. */
inline std::optional<SnapCandidate> FindSettledSnap(std::span<const CS2Kit::Sdk::UserCmdView> window,
                                                    const AimSnapSettings& cfg)
{
    return FindSettledSnap([window](int ago) -> const CS2Kit::Sdk::UserCmdView& { return window[ago]; },
                           static_cast<int>(window.size()), cfg);
}

}  // namespace Anticheat::Detectors::AimSnap
