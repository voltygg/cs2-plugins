#pragma once

#include "AntiCheatManager.hpp"
#include "Config.hpp"
#include "Core/DetectionData.hpp"
#include "Response/DiscordReporter.hpp"
#include "Response/ResponseManager.hpp"

#include <CS2Kit/Api.hpp>

namespace Anticheat
{

/** Subclass CS2Kit::JsonConfig instead once you need post-load validation or accessors. */
using ConfigManager = CS2Kit::JsonConfig<Settings>;

/** The event/convar tables the detections compare against; see Core/DetectionData.hpp. */
using DetectionDataManager = CS2Kit::JsonConfig<DetectionData>;

struct Managers;

/** The plugin's live managers. Valid only between OnLoad and unload. */
Managers& App();

/**
 * Plugin-owned managers, constructed by PluginBase after the kit services are live and
 * destroyed on unload - state cannot leak across `meta reload`. Declared in dependency
 * order; destroyed in reverse.
 */
struct Managers
{
    ConfigManager Config;
    DetectionDataManager Detections;
    DiscordReporter Reporter;
    ResponseManager Response{Reporter};
    AntiCheatManager AntiCheat{Response};
};

}  // namespace Anticheat
