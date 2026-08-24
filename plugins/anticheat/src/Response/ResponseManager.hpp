#pragma once

#include "Config.hpp"
#include "Core/Finding.hpp"
#include "Response/DiscordReporter.hpp"
#include "Response/FunnelPolicy.hpp"

#include <VoltMod/Api.hpp>

namespace Anticheat
{

/**
 * Cores already self-threshold, so this only decides how loudly to react. Bans go through
 * admin-system's Contracts::IAdminActions to keep persistence, kick and broadcast in one place;
 * when that plugin is absent the interface is simply missing and the ban is logged as skipped.
 */
class ResponseManager
{
public:
    ResponseManager(VoltMod::Runtime& runtime, ConfigManager& config, DiscordReporter& reporter)
        : _rt(runtime), _config(config), _reporter(reporter)
    {}

    void Initialize();

    /** Log, report, then apply the funnel decision. */
    void Handle(int slot, const Finding& finding);

    /** Disconnect: a new occupant of the slot starts clean. */
    void OnSlotChanged(int slot);

    /** Map change or config reload. Named to match the detection cores, so the manager can fan out
     *  over all of them at once. */
    void Reset();

    Mode CurrentMode() const;

    /** What has already been done to @p slot this map. */
    PunishmentLevel Issued(int slot) const { return _latch.Level(slot); }

private:
    /** One admin alert per (steamId, detection) per window. */
    static constexpr int64_t AlertThrottleSec = 30;

    bool IsWhitelisted(int64_t steamId) const;

    VoltMod::Runtime& _rt;
    ConfigManager& _config;
    DiscordReporter& _reporter;
    PunishmentLatch _latch;
    VoltMod::PairThrottle<int64_t, int> _alertThrottle{AlertThrottleSec};
};

}  // namespace Anticheat
