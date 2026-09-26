#pragma once

#include "BhopManager.hpp"
#include "Config.hpp"

#include <VoltMod/Api.hpp>

namespace Bhop
{

/** One load cycle's state; members are destroyed in reverse order. */
struct App final : VoltMod::Plugin
{
    using Plugin::Plugin;

    bool Load() override
    {
        Bhop.ApplySettings();
        return true;
    }

    ConfigManager Config = VoltMod::LoadConfig<ConfigManager>(Runtime);
    BhopManager Bhop{Runtime, Config};
};

}  // namespace Bhop
