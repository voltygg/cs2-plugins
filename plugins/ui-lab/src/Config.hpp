#pragma once

#include <VoltMod/Api.hpp>
#include <VoltMod/App/Config.hpp>
#include <string>

namespace UiLab
{

/** The layout this lab drives by default; `uilab_spawn` with no argument uses it. */
struct UiSettings
{
    std::string layout = "ui_lab";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UiSettings, layout)

/** Root of settings.jsonc; add a struct + a member here for each new section. */
struct Settings
{
    VoltMod::StandardPluginSettings plugin;
    UiSettings ui;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, plugin, ui)

/** Subclass VoltMod::JsonConfig instead once you need post-load validation or accessors. */
using ConfigManager = VoltMod::JsonConfig<Settings>;

}  // namespace UiLab
