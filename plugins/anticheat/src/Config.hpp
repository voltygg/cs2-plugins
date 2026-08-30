#pragma once

#include <VoltMod/App/JsonConfig.hpp>

// SDK-free so cores and their tests can include it: JsonConfig is header-only.
// Detection thresholds are deliberately absent - every tuned constant is constexpr in its own core,
// leaving only the operational surface below for an operator to reason about.
#include "Core/Finding.hpp"

#include <cstddef>
#include <cstdint>
#include <iterator>
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

/** The simulator rewrites live player commands: leave it off outside a test box. */
struct DebugSettings
{
    bool simulator = false;
};

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

struct Settings
{
    AntiCheatSettings anticheat;
};

/** Subclass VoltMod::JsonConfig instead once you need post-load validation or accessors. */
using ConfigManager = VoltMod::JsonConfig<Settings>;

}  // namespace Anticheat

/** Accepts the `"$schema"` key settings.jsonc names for editor completion. */
VOLTMOD_SETTINGS_ROOT(Anticheat::Settings)
