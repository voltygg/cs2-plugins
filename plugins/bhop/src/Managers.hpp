#pragma once

#include "BhopManager.hpp"
#include "Config.hpp"

namespace Bhop
{

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
    BhopManager Bhop{Config};
};

}  // namespace Bhop
