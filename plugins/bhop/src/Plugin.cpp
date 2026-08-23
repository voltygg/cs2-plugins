#include "Plugin.hpp"

#include "Config.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/App/PluginInfoStamp.hpp>

CS2KIT_PLUGIN(BhopPlugin);

CS2Kit::PluginInfo BhopPlugin::Info() const
{
    return CS2Kit::WithBuildInfo({
        .Name = "Bhop",
        .Author = "m9snoi",
        .Description = "Smooth, client-predicted bunnyhop with per-player session grants.",
        .LogTag = "BHOP",
    });
}

bool BhopPlugin::OnLoad(CS2Kit::Runtime& runtime, bool late)
{
    _app.emplace(runtime);
    return _app->Start();
}

void BhopPlugin::OnPlayerDisconnect(CS2Kit::Player* player)
{
    _app->Bhop.OnPlayerDisconnect(player);
}
