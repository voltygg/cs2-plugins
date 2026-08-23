#pragma once

#include <CS2Kit/App/JsonConfig.hpp>

// SDK-free so cores and their tests can include it: JsonConfig is header-only over nlohmann.
// Detection thresholds are deliberately absent - every tuned constant is constexpr in its own core,
// leaving only the operational surface below for an operator to reason about.
#include "Core/Finding.hpp"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace Anticheat
{

/** The addon folder name - matches the CMake target and keys every addons/ path. */
inline constexpr std::string_view AddonName = "anticheat";

/** Field names are the JSON keys, so they keep their lowercase spelling. */
struct DetectionToggles
{
    bool aimbot = true;
    bool aimlock = true;
    bool antiAim = true;
    bool silentAim = true;
    bool dllInjection = true;
    bool invalidCvar = true;
    bool namechanger = true;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DetectionToggles, aimbot, aimlock, antiAim, silentAim, dllInjection,
                                                invalidCvar, namechanger)

/** Toggle per DetectionKind, in enum order - the only place the two lists have to agree. */
inline constexpr bool DetectionToggles::* DetectionToggleTable[] = {
    &DetectionToggles::aimbot,      &DetectionToggles::aimlock,      &DetectionToggles::antiAim,
    &DetectionToggles::silentAim,   &DetectionToggles::dllInjection, &DetectionToggles::invalidCvar,
    &DetectionToggles::namechanger,
};
static_assert(std::size(DetectionToggleTable) == static_cast<size_t>(DetectionKind::Count));

inline bool DetectionEnabled(const DetectionToggles& toggles, DetectionKind kind)
{
    return toggles.*DetectionToggleTable[static_cast<size_t>(kind)];
}

/** Inactive while the url is empty. */
struct WebhookSettings
{
    std::string url;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WebhookSettings, url)

/** The simulator rewrites live player commands: leave it off outside a test box. */
struct DebugSettings
{
    bool simulator = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DebugSettings, simulator)

struct AntiCheatSettings
{
    bool enabled = true;
    std::string mode = "observe";  // "observe" | "alert" | "ban"
    int64_t banDurationSec = 0;    // 0 = permanent
    std::vector<int64_t> whitelistSteamIds;
    bool allowSvCheatsTesting = false;
    DetectionToggles detections;
    WebhookSettings webhook;
    DebugSettings debug;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AntiCheatSettings, enabled, mode, banDurationSec, whitelistSteamIds,
                                                allowSvCheatsTesting, detections, webhook, debug)

struct Settings
{
    AntiCheatSettings anticheat;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, anticheat)

/** Subclass CS2Kit::JsonConfig instead once you need post-load validation or accessors. */
using ConfigManager = CS2Kit::App::JsonConfig<Settings>;

}  // namespace Anticheat
