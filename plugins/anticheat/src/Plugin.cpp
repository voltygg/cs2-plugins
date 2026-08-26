#include "Plugin.hpp"

#include "Config.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginInfoStamp.hpp>

VOLTMOD_PLUGIN(AnticheatPlugin);

VoltMod::PluginInfo AnticheatPlugin::Info() const
{
    return VoltMod::WithBuildInfo({
        .Name = "Anticheat",
        .Author = "Sukhrob Ilyosbekov",
        .Description = "Server-side cheat detection: aim analysis over correlated shots plus client-integrity checks.",
        .LogTag = "ANTICHEAT",
    });
}

bool AnticheatPlugin::OnLoad(VoltMod::Runtime& runtime, bool /*late*/)
{
    _app.emplace(runtime);
    return _app->Start();
}

void AnticheatPlugin::OnServerStartup(const char* /*mapName*/)
{
    _app->AntiCheat.OnMapStart();
}
