#include "Plugin.hpp"

#include "Config.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginInfoStamp.hpp>

VOLTMOD_PLUGIN(HudLabPlugin);

VoltMod::PluginInfo HudLabPlugin::Info() const
{
    return VoltMod::WithBuildInfo({
        .Name = "Hud Lab",
        .Author = "Sukhrob Ilyosbekov",
        .Description = "Experiments with CS2 native custom_hud_layout Panorama UI.",
        .LogTag = "HUDLAB",
    });
}

bool HudLabPlugin::OnLoad(VoltMod::Runtime& runtime)
{
    _app.emplace(runtime);
    return _app->Start();
}
