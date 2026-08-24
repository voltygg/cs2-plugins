#include "Plugin.hpp"

#include "Config.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginInfoStamp.hpp>

VOLTMOD_PLUGIN(BhopPlugin);

VoltMod::PluginInfo BhopPlugin::Info() const
{
    return VoltMod::WithBuildInfo({
        .Name = "Bhop",
        .Author = "m9snoi",
        .Description = "Smooth, client-predicted bunnyhop with per-player session grants.",
        .LogTag = "BHOP",
    });
}

bool BhopPlugin::OnLoad(VoltMod::Runtime& runtime, bool late)
{
    _app.emplace(runtime);
    return _app->Start();
}

void BhopPlugin::OnPlayerDisconnect(VoltMod::Player* player)
{
    _app->Bhop.OnPlayerDisconnect(player);
}
