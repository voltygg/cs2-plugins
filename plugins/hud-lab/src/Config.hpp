#pragma once

#include <VoltMod/Api.hpp>
#include <VoltMod/App/Config.hpp>
#include <string>

namespace HudLab
{

/** The layout this lab drives by default; `hudlab_spawn` with no argument uses it. */
struct HudSettings
{
    std::string layout = "hud_lab";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HudSettings, layout)

/** Root of settings.jsonc; add a struct + a member here for each new section. */
struct Settings
{
    VoltMod::StandardPluginSettings plugin;
    HudSettings hud;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, plugin, hud)

/** Subclass VoltMod::JsonConfig instead once you need post-load validation or accessors. */
using ConfigManager = VoltMod::JsonConfig<Settings>;

}  // namespace HudLab
