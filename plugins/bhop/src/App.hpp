#pragma once

#include "BhopManager.hpp"
#include "Config.hpp"

#include <VoltMod/Api.hpp>

namespace Bhop
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

    /** Load settings and start the bhop policy. False aborts the plugin load. */
    bool Start();

    VoltMod::Runtime& Runtime;
    ConfigManager Config;
    BhopManager Bhop{Runtime, Config};
};

}  // namespace Bhop
