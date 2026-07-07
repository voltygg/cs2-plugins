#pragma once

#include <CS2Kit/Api.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace Anticheat
{

inline constexpr const char* SettingsPath = "addons/anticheat/configs/settings.jsonc";

/** "plugin" section of settings.jsonc. */
struct PluginSettings
{
    std::string logLevel = "info";
    std::string locale = "en";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PluginSettings, logLevel, locale)

struct BanSettings
{
    int64_t durationSec = 0;  // 0 = permanent
    std::string reason = "AntiCheat: blatant input anomaly";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BanSettings, durationSec, reason)

/**
 * Spinbot. Spinning alone never bans - legit players fake-spin for fun. The ban
 * signal is repeated kills whose fire-tick aim snapped onto the victim mid-spin.
 */
struct SpinSettings
{
    bool enabled = true;
    float yawVelocityDegPerSec = 1800.0f;  // window mean |yaw velocity| that counts as spinning
    int minTicks = 32;                     // sustained ticks before the spin state arms
    int killWindowTicks = 8;               // kill counts as "mid-spin" this many ticks after spinning
    float onTargetEpsilonDeg = 8.0f;       // fire-tick angle must land this close to the victim
    int minKillEvents = 3;                 // distinct spin-snap-kills required for ban tier
    float eventWindowSec = 120.0f;
    float spinOnlyScore = 5.0f;  // observe-tier only; capped below alertScore by design
    float killEventScore = 40.0f;
    float headshotMultiplier = 1.5f;
    float alertScore = 60.0f;
    float banScore = 110.0f;
    float decayPerSec = 0.5f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpinSettings, enabled, yawVelocityDegPerSec, minTicks, killWindowTicks,
                                                onTargetEpsilonDeg, minKillEvents, eventWindowSec, spinOnlyScore,
                                                killEventScore, headshotMultiplier, alertScore, banScore, decayPerSec)

/** Aimlock/aim-snap: big pre-fire flick that settles, confirmed only by on-target damage. */
struct AimSnapSettings
{
    bool enabled = true;
    int lookbackTicks = 16;
    float minSnapDeg = 40.0f;
    float settleEpsilonDeg = 1.0f;  // post-snap per-tick motion that still counts as "locked"
    int confirmWindowTicks = 4;     // damage must arrive this close to the fire tick
    float onTargetEpsilonDeg = 8.0f;
    int minEvents = 4;  // confirmed snap-hits required for ban tier
    float eventWindowSec = 180.0f;
    float snapScore = 3.0f;  // unconfirmed snaps: observe log only
    float confirmedHitScore = 25.0f;
    float alertScore = 60.0f;
    float banScore = 100.0f;
    float decayPerSec = 0.5f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AimSnapSettings, enabled, lookbackTicks, minSnapDeg, settleEpsilonDeg,
                                                confirmWindowTicks, onTargetEpsilonDeg, minEvents, eventWindowSec,
                                                snapScore, confirmedHitScore, alertScore, banScore, decayPerSec)

/** NaN/inf view angles or pitch outside the legal range - impossible via normal input. */
struct SanitySettings
{
    bool enabled = true;
    float maxPitchDeg = 89.5f;
    int minTicks = 4;  // consecutive bad ticks before reporting
    int minEvents = 1;
    float eventWindowSec = 60.0f;
    float eventScore = 50.0f;
    float alertScore = 50.0f;
    float banScore = 50.0f;
    float decayPerSec = 0.1f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SanitySettings, enabled, maxPitchDeg, minTicks, minEvents,
                                                eventWindowSec, eventScore, alertScore, banScore, decayPerSec)

/** Silent aim: viewangle jumps with no matching mouse input, confirmed by on-target damage. */
struct SilentAimSettings
{
    bool enabled = true;
    float minAngleJumpDeg = 20.0f;
    int maxMouseUnits = 2;  // |mousedx|+|mousedy| at or below this counts as "no mouse input"
    int confirmWindowTicks = 4;
    float onTargetEpsilonDeg = 8.0f;
    int minEvents = 3;
    float eventWindowSec = 180.0f;
    float eventScore = 35.0f;
    float alertScore = 60.0f;
    float banScore = 100.0f;
    float decayPerSec = 0.5f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SilentAimSettings, enabled, minAngleJumpDeg, maxMouseUnits,
                                                confirmWindowTicks, onTargetEpsilonDeg, minEvents, eventWindowSec,
                                                eventScore, alertScore, banScore, decayPerSec)

/** No-flash: repeated kills while fully blinded (tracked via the player_blind event). */
struct NoFlashSettings
{
    bool enabled = true;
    float minRemainingSec = 1.0f;  // blind time still remaining at the kill
    int minEvents = 3;
    float eventWindowSec = 120.0f;
    float eventScore = 35.0f;
    float alertScore = 60.0f;
    float banScore = 100.0f;
    float decayPerSec = 0.5f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NoFlashSettings, enabled, minRemainingSec, minEvents, eventWindowSec,
                                                eventScore, alertScore, banScore, decayPerSec)

struct DetectorSettings
{
    SpinSettings spin;
    AimSnapSettings aimSnap;
    SanitySettings sanity;
    SilentAimSettings silentAim;
    NoFlashSettings noFlash;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DetectorSettings, spin, aimSnap, sanity, silentAim, noFlash)

/** "anticheat" section: response ladder + detector thresholds. */
struct AntiCheatSettings
{
    std::string mode = "observe";  // "observe" | "alert" | "ban"
    float alertCooldownSec = 30.0f;
    int historyDepth = 128;  // usercmd lookback samples per player (~2s at 64 tick)
    BanSettings ban;
    DetectorSettings detectors;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AntiCheatSettings, mode, alertCooldownSec, historyDepth, ban, detectors)

/** Root of settings.jsonc. Add a struct + a member here for each new section. */
struct Settings
{
    PluginSettings plugin;
    AntiCheatSettings anticheat;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, plugin, anticheat)

/** Subclass CS2Kit::JsonConfig instead once you need post-load validation or accessors. */
using ConfigManager = CS2Kit::JsonConfig<Settings>;

}  // namespace Anticheat
