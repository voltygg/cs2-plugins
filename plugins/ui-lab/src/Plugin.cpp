#include "Plugin.hpp"

#include "Config.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginInfoStamp.hpp>

VOLTMOD_PLUGIN(UiLabPlugin);

VoltMod::PluginInfo UiLabPlugin::Info() const
{
    return VoltMod::WithBuildInfo({
        .Name = "UI Lab",
        .Author = "Sukhrob Ilyosbekov",
        .Description = "Experiments with CS2 native custom_hud_layout Panorama UI.",
        .LogTag = "UILAB",
    });
}

bool UiLabPlugin::OnLoad(VoltMod::Runtime& runtime)
{
    _app.emplace(runtime);
    return _app->Start();
}
