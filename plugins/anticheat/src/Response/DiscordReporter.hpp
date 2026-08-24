#pragma once

#include "Config.hpp"
#include "Core/Finding.hpp"
#include "Response/FunnelPolicy.hpp"

#include <VoltMod/Api.hpp>
#include <cstdint>
#include <string>

namespace Anticheat
{

/**
 * Fire-and-forget Discord embed per detection. Dormant while `anticheat.webhook.url` is empty, and
 * failures log once without retrying: reporting must never affect detection.
 */
class DiscordReporter
{
public:
    DiscordReporter(VoltMod::Runtime& runtime, ConfigManager& config) : _rt(runtime), _config(config) {}

    void Report(int slot, const std::string& playerName, int64_t steamId, const Finding& finding,
                FunnelOutcome outcome);

private:
    /** One embed per (steamId, detection) per window, so no detection can flood a channel. */
    static constexpr int64_t ThrottleSec = 60;

    VoltMod::Runtime& _rt;
    ConfigManager& _config;
    VoltMod::PairThrottle<int64_t, int> _throttle{ThrottleSec};
};

}  // namespace Anticheat
