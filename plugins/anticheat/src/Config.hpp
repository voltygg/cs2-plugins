#pragma once

#include <VoltMod/App/Config/Options.hpp>

#include "Detect/Finding.hpp"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace Anticheat
{
/** Field names are the JSON keys, so they keep their lowercase spelling. */
struct DetectionToggles
{
    bool aimbot = true;
    bool aimlock = true;
    bool antiAim = true;
    bool silentAim = true;
    bool triggerbot = true;
    bool recoil = true;
    bool aimAssist = true;
    bool wallhack = true;
    bool dllInjection = true;
    bool invalidCvar = true;
    bool namechanger = true;
};

/** Toggle per DetectionKind, in enum order - the only place the two lists have to agree. */
inline constexpr bool DetectionToggles::* DetectionToggleTable[] = {
    &DetectionToggles::aimbot,     &DetectionToggles::aimlock,       &DetectionToggles::antiAim,
    &DetectionToggles::silentAim,  &DetectionToggles::triggerbot,    &DetectionToggles::recoil,
    &DetectionToggles::aimAssist, &DetectionToggles::wallhack,   &DetectionToggles::dllInjection,
    &DetectionToggles::invalidCvar, &DetectionToggles::namechanger,
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

/** Test-box switches: the simulator rewrites live player commands, and including bots lets a
 *  headless server exercise the whole detection path. Leave both off in production. */
struct DebugSettings
{
    bool simulator = false;
    bool includeBots = false;
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

/** Give Options a snapshot type and a builder once you need post-load validation or accessors. */
using ConfigManager = VoltMod::Options<Settings>;

}  // namespace Anticheat

/** Accepts the `"$schema"` key settings.jsonc names for editor completion. */
