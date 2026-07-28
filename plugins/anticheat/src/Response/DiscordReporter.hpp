#pragma once

#include "Core/Finding.hpp"
#include "Response/FunnelPolicy.hpp"

#include <CS2Kit/Api.hpp>
#include <cstdint>
#include <string>

namespace Anticheat
{

/**
 * Fire-and-forget Discord embed per detection. Dormant while `anticheat.webhook.url` is empty;
 * failures log once and are never retried, because reporting must not affect detection.
 */
class DiscordReporter
{
public:
    void Report(int slot, const std::string& playerName, int64_t steamId, const Finding& finding,
                FunnelOutcome outcome);

private:
    /** One embed per (steamId, detection) per minute, so a spamming detection cannot flood a channel. */
    static constexpr int64_t ThrottleSec = 60;

    CS2Kit::PairThrottle<int64_t, int> _throttle{ThrottleSec};
};

}  // namespace Anticheat
