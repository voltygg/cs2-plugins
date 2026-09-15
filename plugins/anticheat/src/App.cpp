#include "App.hpp"

#include <VoltMod/Api.hpp>

namespace Anticheat
{

bool App::Start()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Config, {.Addon = AddonName, .Translations = false}))
        return false;

    // A missing data file leaves the two table-driven modules inert rather than taking the plugin
    // down: the aim modules, which carry no data file, are the ones worth keeping alive.
    Runtime.LoadSteps.Optional("Detection data", [this] {
        VoltMod::Status loaded = Detections.Load(DetectionDataPath);
        if (!loaded)
            loaded.error().Detail += "; DLL injection and invalid cvar modules are inert";
        return loaded;
    });

    Response.Initialize();
    AntiCheat.Initialize();

    VoltMod::Log::Info("Mode: {}.", Config.Get().anticheat.mode);
    return true;
}

}  // namespace Anticheat
