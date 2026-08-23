#include "Plugin.hpp"

#include "Config.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/App/PluginInfoStamp.hpp>

CS2KIT_PLUGIN(AnticheatPlugin);

CS2Kit::PluginInfo AnticheatPlugin::Info() const
{
    return CS2Kit::WithBuildInfo({
        .Name = "Anticheat",
        .Author = "m9snoi",
        .Description = "Server-side cheat detection: aim analysis over correlated shots plus client-integrity checks.",
        .LogTag = "ANTICHEAT",
    });
}

bool AnticheatPlugin::OnLoad(CS2Kit::Runtime& runtime, bool /*late*/)
{
    _app.emplace(runtime);
    return _app->Start();
}

void AnticheatPlugin::OnServerStartup(const char* /*mapName*/)
{
    _app->AntiCheat.OnMapStart();
}

void AnticheatPlugin::OnPlayerFullyConnected(CS2Kit::Player* player)
{
    _app->AntiCheat.OnPlayerFullyConnected(player);
}

void AnticheatPlugin::OnPlayerSettingsChanged(CS2Kit::Player* player)
{
    _app->AntiCheat.OnPlayerSettingsChanged(player);
}
