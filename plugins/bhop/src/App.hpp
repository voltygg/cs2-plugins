#pragma once

#include "BhopManager.hpp"
#include "Config.hpp"

#include <VoltMod/Api.hpp>

namespace Bhop
{

/** One load cycle's state; members are destroyed in reverse order. */
struct App final : VoltMod::Plugin
{
    explicit App(VoltMod::Runtime& runtime) : Plugin(runtime) {}

    /** Loads settings and starts the bhop policy. False aborts the load. */
    bool Load() override;

    ConfigManager Config;
    BhopManager Bhop{Runtime, Config};
};

}  // namespace Bhop
