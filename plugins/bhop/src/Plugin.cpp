#include "Plugin.hpp"

#include "Config.hpp"
#include "Managers.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/PluginInfoStamp.hpp>

CS2KIT_PLUGIN(BhopPlugin, Bhop);

CS2Kit::PluginInfo BhopPlugin::Info() const
{
    return CS2Kit::WithBuildInfo({
        .Name = "Bhop",
        .Author = "m9snoi",
        .Description = "Smooth, client-predicted bunnyhop with per-player session grants.",
        .LogTag = "BHOP",
    });
}

bool BhopPlugin::OnLoad(bool late)
{
    if (!CS2Kit::LoadStandardConfig(Bhop::App().Config, {.Addon = Bhop::AddonName}))
        return false;

    Bhop::App().Bhop.Initialize();
    return true;
}

void BhopPlugin::OnPlayerDisconnect(CS2Kit::Player* player)
{
    Bhop::App().Bhop.OnPlayerDisconnect(player);
}
