#include "Plugin.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginInfoStamp.hpp>

VOLTMOD_PLUGIN(UiPlugin);

VoltMod::PluginInfo UiPlugin::Info() const
{
    return VoltMod::WithBuildInfo({
        .Name = "Server UI",
        .Author = "meat.gg",
        .Description = "Panorama HUD other plugins draw on through Contracts::IUiHud",
        .LogTag = "UI",
    });
}

bool UiPlugin::OnLoad(VoltMod::Runtime& runtime)
{
    _app.emplace(runtime);
    return _app->Start();
}
