#pragma once

#include <VoltMod/Api.hpp>
#include <VoltMod/App/Config.hpp>
#include <cstdint>
#include <string>
#include <string_view>

namespace Ui
{

inline constexpr std::string_view AddonName = "ui";

/** The `ui` object in settings.jsonc. */
struct HudSettings
{
    /** Workshop addon carrying the compiled layout. 0 requires nothing of connecting clients,
     *  which is what you want while copying the files into your own client by hand. */
    uint64_t addonId = 0;
    std::string serverName = "meat.gg";
    int toastDurationMs = 4000;
};

/** Root of settings.jsonc. Public members are reflected, so the member name is the JSON key. */
struct Settings
{
    VoltMod::StandardPluginSettings plugin;
    HudSettings ui;
};

using ConfigManager = VoltMod::JsonConfig<Settings>;

}  // namespace Ui
