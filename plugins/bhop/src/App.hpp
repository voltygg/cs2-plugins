#pragma once

#include "BhopManager.hpp"
#include "Config.hpp"

#include <VoltMod/Api.hpp>

namespace Bhop
{

/**
 * Everything this plugin owns for one load cycle. VoltMod destroys it before the runtime, so no
 * state survives a `volt reload`.
 *
 * Members are declared in dependency order and destroyed in reverse; each is handed the
 * collaborators it needs.
 */
struct App final : VoltMod::Plugin
{
    explicit App(VoltMod::Runtime& runtime) : Plugin(runtime) {}

    /** Load settings and start the bhop policy. False aborts the plugin load. */
    bool Load() override;

    ConfigManager Config;
    BhopManager Bhop{Runtime, Config};
};

}  // namespace Bhop
