#include "Plugin.hpp"

#include "Config.hpp"
#include "Managers.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/PluginInfoStamp.hpp>

using CS2Kit::Core::Engine;

CS2KIT_PLUGIN(AnticheatPlugin, Anticheat);

CS2Kit::PluginInfo AnticheatPlugin::Info() const
{
    return CS2Kit::WithBuildInfo({
        .Name = "Anticheat",
        .Author = "m9snoi",
        .Description = "Server-side cheat detection: aim analysis over correlated shots plus client-integrity checks.",
        .LogTag = "ANTICHEAT",
    });
}

bool AnticheatPlugin::OnLoad(bool /*late*/)
{
    if (!CS2Kit::LoadStandardConfig(Anticheat::App().Config, {.Addon = Anticheat::AddonName, .Translations = false}))
        return false;

    // A missing data file leaves the two table-driven modules inert rather than taking the plugin
    // down: the aim modules, which carry no data file, are the ones worth keeping alive.
    Engine().LoadReport.Run("Detection data", [] {
        if (!Anticheat::App().Detections.Load(Anticheat::DetectionDataPath))
            return CS2Kit::StageResult::Degraded("file unreadable; DLL injection and invalid cvar modules are inert");
        return CS2Kit::StageResult::Ok(Anticheat::DetectionDataPath);
    });

    Anticheat::App().Response.Initialize();
    Anticheat::App().AntiCheat.Initialize();

    CS2Kit::Log::Info("Mode: {}.", Anticheat::App().Config.Get().anticheat.mode);
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
