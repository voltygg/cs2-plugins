#pragma once

#include "Core/Finding.hpp"
#include "Response/DiscordReporter.hpp"
#include "Response/FunnelPolicy.hpp"

#include <CS2Kit/Api.hpp>

namespace Anticheat
{

/**
 * The violation funnel. Cores already self-threshold, so a Finding is a verdict: this only decides
 * how loudly to react. Bans go through admin-system's console bridge (as_ac_ban) so persistence,
 * kick and broadcast stay in one place.
 */
class ResponseManager
{
public:
    explicit ResponseManager(DiscordReporter& reporter) : _reporter(reporter) {}

    void Initialize();

    /** Log, report, then apply the funnel decision. */
    void Handle(int slot, const Finding& finding);

    /** Disconnect: a new occupant of the slot starts clean. */
    void OnSlotChanged(int slot);

    /** Map change or config reload. */
    void ResetAll();

    Mode CurrentMode() const;

    /** What has already been done to @p slot this map. */
    PunishmentLevel Issued(int slot) const { return _latch.Level(slot); }

private:
    /** One admin alert per (steamId, detection) per half minute. */
    static constexpr int64_t AlertThrottleSec = 30;

    bool IsWhitelisted(int64_t steamId) const;

    DiscordReporter& _reporter;
    PunishmentLatch _latch;
    CS2Kit::PairThrottle<int64_t, int> _alertThrottle{AlertThrottleSec};
};

}  // namespace Anticheat
