#pragma once

#include "Config.hpp"
#include "Detect/Finding.hpp"
#include "Response/DiscordReporter.hpp"
#include "Response/ResponsePolicy.hpp"

#include <VoltMod/Api.hpp>

namespace Anticheat
{

/** Suspicion decides whether to speak; this decides how loudly. Bans go through admin-system's
 *  Contracts::IAdminActions. Nothing counts as punished until it lands, so a failure stays
 *  retryable. */
class ResponseManager
{
public:
    ResponseManager(VoltMod::Runtime& runtime, ConfigManager& config, DiscordReporter& reporter)
        : _rt(runtime), _config(config), _reporter(reporter)
    {}

    /** Alert, apply the funnel decision, then record what actually happened. */
    void Handle(int slot, const Finding& finding);

    Mode CurrentMode() const;

    /** What has actually been done to @p steamId while the server has been up. */
    PunishmentLevel Issued(int64_t steamId) const { return _issued.Level(steamId); }

private:
    bool IsWhitelisted(int64_t steamId) const;

    /** Log and report the finding under the outcome it ended with, once any punishment has
     *  resolved. */
    void Record(int slot, const std::string& name, int64_t steamId, const Finding& finding, ResponseOutcome outcome);

    /** Both raise the issued level only on success. */
    ResponseOutcome ApplyKick(int slot, int64_t steamId, const std::string& reason);
    ResponseOutcome ApplyBan(int64_t steamId, const std::string& reason);

    VoltMod::Runtime& _rt;
    ConfigManager& _config;
    DiscordReporter& _reporter;
    IssuedPunishments _issued;
    /** Deferred kick per slot: a new one replaces whatever was pending, and unload cancels it. */
    VoltMod::PerSlot<VoltMod::Subscription> _pendingKick;
};

}  // namespace Anticheat
