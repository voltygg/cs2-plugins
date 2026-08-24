#pragma once

#include "AntiCheatManager.hpp"
#include "Config.hpp"
#include "Core/DetectionData.hpp"
#include "Response/DiscordReporter.hpp"
#include "Response/ResponseManager.hpp"

#include <VoltMod/Api.hpp>

namespace Anticheat
{

/**
 * Everything this plugin owns for one Load/Unload cycle. The plugin creates it in OnLoad and
 * drops it in OnUnload, so no state survives a `meta reload`.
 *
 * Members are declared in dependency order and destroyed in reverse; each is handed the
 * collaborators it needs.
 */
struct App
{
    explicit App(VoltMod::Runtime& runtime) : Runtime(runtime) {}

    /** Load config and detection data, then arm the detection modules. */
    bool Start();

    VoltMod::Runtime& Runtime;
    ConfigManager Config;
    DetectionDataManager Detections;
    DiscordReporter Reporter{Runtime, Config};
    ResponseManager Response{Runtime, Config, Reporter};
    AntiCheatManager AntiCheat{Runtime, Config, Detections, Response};
};

}  // namespace Anticheat
