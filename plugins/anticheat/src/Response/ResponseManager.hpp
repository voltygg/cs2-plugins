#pragma once

#include "Config.hpp"
#include "Detect/Finding.hpp"
#include "Response/DiscordReporter.hpp"
#include "Response/ResponsePolicy.hpp"

#include <VoltMod/Api.hpp>

namespace Anticheat
{

/**
 * Suspicion decides whether to speak; this only decides how loudly. Bans go through admin-system's
 * Contracts::IAdminActions, so persistence, kick and broadcast stay in one place; when that plugin
 * is absent the interface is missing and the ban is logged as skipped.
 */
class ResponseManager
{
public:
    ResponseManager(VoltMod::Runtime& runtime, ConfigManager& config, DiscordReporter& reporter)
        : _rt(runtime), _config(config), _reporter(reporter)
    {}

    /** Log, report, then apply the funnel decision. */
    void Handle(int slot, const Finding& finding);

    Mode CurrentMode() const;

    /** What has already been done to @p steamId while the server has been up. */
    PunishmentLevel Issued(int64_t steamId) const { return _issued.Level(steamId); }

private:
    bool IsWhitelisted(int64_t steamId) const;

    VoltMod::Runtime& _rt;
    ConfigManager& _config;
    DiscordReporter& _reporter;
    IssuedPunishments _issued;
    /** Deferred kick per slot: a new one replaces whatever was pending, and unload cancels it. */
    VoltMod::PerSlot<VoltMod::Subscription> _pendingKick;
};

}  // namespace Anticheat
