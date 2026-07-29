#include "Plugin.hpp"

#include "Config.hpp"
#include "Managers.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/BuildInfo.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Utils/Log.hpp>

using CS2Kit::Core::Engine;
namespace Log = CS2Kit::Utils::Log;

AnticheatPlugin g_AnticheatPlugin;
PLUGIN_EXPOSE(AnticheatPlugin, g_AnticheatPlugin);

namespace Anticheat
{
Managers& App()
{
    return AnticheatPlugin::App();
}
}  // namespace Anticheat

CS2Kit::PluginInfo AnticheatPlugin::Info() const
{
    return CS2Kit::PluginInfo{
        .Name = "Anticheat",
        .Author = "m9snoi",
        .Description = "Server-side cheat detection: aim analysis over correlated shots plus client-integrity checks.",
        .Url = "",
        .License = "MIT",
        .Version = CS2Kit::BuildInfo::Version,
        .Date = CS2Kit::BuildInfo::BuildDate,
        .Commit = CS2Kit::BuildInfo::RepoCommit,
        .LogTag = "ANTICHEAT",
    };
}

bool AnticheatPlugin::OnLoad(bool /*late*/)
{
    if (!Anticheat::App().Config.Load(Anticheat::SettingsPath))
        return false;
    // A missing data file leaves the two table-driven modules inert rather than taking the plugin
    // down: the aim modules, which carry no data file, are the ones worth keeping alive.
    if (!Anticheat::App().Detections.Load(Anticheat::DetectionDataPath))
        Log::Warn("{} could not be read; the DLL injection and invalid cvar modules have nothing to check against.",
                  Anticheat::DetectionDataPath);

    Anticheat::App().Response.Initialize();
    Anticheat::App().AntiCheat.Initialize();

    Log::Info("Loaded v{} (mode={}).", Info().Version, Anticheat::App().Config.Get().anticheat.mode);
    return true;
}

void AnticheatPlugin::OnServerStartup(const char* /*mapName*/)
{
    Anticheat::App().AntiCheat.OnMapStart();
}

void AnticheatPlugin::OnPlayerFullyConnected(CS2Kit::Players::Player* player)
{
    Anticheat::App().AntiCheat.OnPlayerFullyConnected(player);
}

void AnticheatPlugin::OnPlayerSettingsChanged(CS2Kit::Players::Player* player)
{
    Anticheat::App().AntiCheat.OnPlayerSettingsChanged(player);
}
